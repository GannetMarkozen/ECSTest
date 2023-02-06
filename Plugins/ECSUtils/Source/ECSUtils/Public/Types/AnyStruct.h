
#pragma once

#include "CoreMinimal.h"
#include "AnyStruct.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ECSUTILS_API FAnyStruct
{
	GENERATED_BODY()

	constexpr FAnyStruct();
	FAnyStruct(const FAnyStruct& Other);
	constexpr FAnyStruct(FAnyStruct&& Other) noexcept;
	explicit FAnyStruct(const UScriptStruct* InType);
	explicit FAnyStruct(const UScriptStruct* InType, const void* InCopy);
	explicit FAnyStruct(ENoInit);
	~FAnyStruct();

	FAnyStruct& operator=(const FAnyStruct& Other);
	FAnyStruct& operator=(FAnyStruct&& Other) noexcept;

	bool operator==(const FAnyStruct& Other) const;
	bool operator!=(const FAnyStruct& Other) const;

	template<typename T>
	typename TEnableIf<!TOr<TIsSame<T,FAnyStruct>, TIsPointer<T>>::Value, bool>::Type operator==(const T& Other) const;

	template<typename T>
	typename TEnableIf<!TOr<TIsSame<T,FAnyStruct>, TIsPointer<T>>::Value, bool>::Type operator!=(const T& Other) const;

	void Destroy();

	template<typename T, typename... ParamTypes>
	static FAnyStruct Make(ParamTypes&&... Params);

	template<typename T, typename... ParamTypes>
	T& Set(ParamTypes&&... Params);

	void Set(const UScriptStruct* InType);
	void Set(const UScriptStruct* InType, const void* InCopy);

	template<typename T>
	T* Get();

	template<typename T>
	const T* Get() const;

	template<typename T>
	T& GetChecked();

	template<typename T>
	const T& GetChecked() const;

	bool IsValid() const;

	bool IsA(const UScriptStruct* InType) const;
	bool IsAExact(const UScriptStruct* InType) const;

	template<typename T>
	bool IsA() const;

	template<typename T>
	bool IsAExact() const;

	//~ Type traits
	void AddStructReferencedObjects(FReferenceCollector& Collector);
	bool Serialize(FArchive& Ar);
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
	//~

private:
	uint8* RawMemory;
	const UScriptStruct* Type;
};

template<>
struct TStructOpsTypeTraits<FAnyStruct> : TStructOpsTypeTraitsBase2<FAnyStruct>
{
	enum
	{
		WithCopy = true,
		WithIdenticalViaEquality = true,
		WithStructReferencedObjects = true,
		WithSerializer = true,
		WithNetSerializer = true,
	};
};

/**
 *
 */

FORCEINLINE constexpr FAnyStruct::FAnyStruct()
	: RawMemory(nullptr), Type(nullptr) {}

FORCEINLINE FAnyStruct::FAnyStruct(const FAnyStruct& Other)
	: FAnyStruct(Other.Type, Other.RawMemory) {}

FORCEINLINE constexpr FAnyStruct::FAnyStruct(FAnyStruct&& Other) noexcept
	: RawMemory(Other.RawMemory), Type(Other.Type)
{
	Other.RawMemory = nullptr;
	Other.Type = nullptr;
}

FORCEINLINE FAnyStruct::FAnyStruct(ENoInit) {}

inline FAnyStruct::FAnyStruct(const UScriptStruct* InType)
{
	if (!InType)
	{
		RawMemory = nullptr;
		Type = nullptr;
		return;
	}

	Type = InType;
	RawMemory = (uint8*)FMemory::Malloc(Type->GetStructureSize(), Type->GetMinAlignment());
	Type->InitializeStruct(RawMemory);
}

FORCEINLINE FAnyStruct::FAnyStruct(const UScriptStruct* InType, const void* InCopy)
	: FAnyStruct(InType)
{
	if (!Type) return;
	check(InCopy);
	Type->CopyScriptStruct(RawMemory, InCopy);
}

FORCEINLINE FAnyStruct::~FAnyStruct()
{
	if (!IsValid()) return;

	Type->DestroyStruct(RawMemory);
	FMemory::Free(RawMemory);
}

FORCEINLINE FAnyStruct& FAnyStruct::operator=(const FAnyStruct& Other)
{
	if (!Other.IsValid())
	{
		Destroy();
		return *this;
	}

	Set(Other.Type, Other.RawMemory);
	return *this;
}

FORCEINLINE FAnyStruct& FAnyStruct::operator=(FAnyStruct&& Other) noexcept
{
	Destroy();
	RawMemory = Other.RawMemory;
	Type = Other.Type;
	Other.RawMemory = nullptr;
	Other.Type = nullptr;
	return *this;
}

inline bool FAnyStruct::operator==(const FAnyStruct& Other) const
{
	if (IsValid() != Other.IsValid()) return false;
	if (!IsValid()) return true;
	if (Type != Other.Type) return false;
	return Type->CompareScriptStruct(RawMemory, Other.RawMemory, EPropertyPortFlags::PPF_None);
}

template<typename T>
FORCEINLINE typename TEnableIf<!TOr<TIsSame<T,FAnyStruct>, TIsPointer<T>>::Value, bool>::Type FAnyStruct::operator==(const T& Other) const
{
	return IsValid() && Type == TBaseStructure<T>::Get() && Type->CompareScriptStruct(RawMemory, &Other, EPropertyPortFlags::PPF_None);
}

template<typename T>
FORCEINLINE typename TEnableIf<!TOr<TIsSame<T,FAnyStruct>, TIsPointer<T>>::Value, bool>::Type FAnyStruct::operator!=(const T& Other) const
{
	return !(*this == Other);
}

FORCEINLINE bool FAnyStruct::operator!=(const FAnyStruct& Other) const
{
	return !(*this == Other);
}

inline void FAnyStruct::Destroy()
{
	if (!IsValid()) return;

	Type->DestroyStruct(RawMemory);
	FMemory::Free(RawMemory);
	RawMemory = nullptr;
	Type = nullptr;
}

template<typename T, typename... ParamTypes>
UE_NODISCARD FORCEINLINE FAnyStruct FAnyStruct::Make(ParamTypes&&... Params)
{
	FAnyStruct Out(NoInit);
	Out.Type = TBaseStructure<T>::Get();
	Out.RawMemory = (uint8*)FMemory::Malloc(sizeof(T), alignof(T));
	new (Out.RawMemory) T(Forward<ParamTypes>(Params)...);
	return Out;
}

template<typename T, typename... ParamTypes>
inline T& FAnyStruct::Set(ParamTypes&&... Params)
{
	const UScriptStruct* NewType = TBaseStructure<T>::Get();
	if (IsValid())
	{
		const bool bSameType = Type == NewType;
		if (bSameType)
		{
			((T*)RawMemory)->~T();
		}
		else
		{
			Type->DestroyStruct(RawMemory);
		}
		
		if (!bSameType || Type->GetStructureSize() != sizeof(T) || Type->GetMinAlignment() != alignof(T))
		{
			RawMemory = (uint8*)FMemory::Realloc(RawMemory, sizeof(T), alignof(T));
		}
	}
	else
	{
		RawMemory = (uint8*)FMemory::Malloc(sizeof(T), alignof(T));
	}

	Type = NewType;
	new (RawMemory) T(Forward<ParamTypes>(Params)...);
	return *(T*)RawMemory;
}

inline void FAnyStruct::Set(const UScriptStruct* InType)
{
	check(InType != nullptr);

	const int32 NewSize = InType->GetStructureSize();
	const int32 NewAlignment = InType->GetMinAlignment();

	if (IsValid())
	{
		Type->DestroyStruct(RawMemory);
		if (Type != InType || Type->GetStructureSize() != NewSize || Type->GetMinAlignment() != NewAlignment)
		{
			RawMemory = (uint8*)FMemory::Realloc(RawMemory, NewSize, NewAlignment);
		}
	}
	else
	{
		RawMemory = (uint8*)FMemory::Malloc(NewSize, NewAlignment);
	}

	Type = InType;
	Type->InitializeStruct(RawMemory);
}

inline void FAnyStruct::Set(const UScriptStruct* InType, const void* InCopy)
{
	check(InCopy != nullptr);
	Set(InType);
	Type->CopyScriptStruct(RawMemory, InCopy);
}

UE_NODISCARD FORCEINLINE bool FAnyStruct::IsValid() const
{
	return Type != nullptr;
}

UE_NODISCARD FORCEINLINE bool FAnyStruct::IsA(const UScriptStruct* InType) const
{
	return IsValid() && InType && Type->IsChildOf(InType);
}

UE_NODISCARD FORCEINLINE bool FAnyStruct::IsAExact(const UScriptStruct* InType) const
{
	return IsValid() && Type == InType;
}

template<typename T>
UE_NODISCARD FORCEINLINE bool FAnyStruct::IsA() const
{
	return IsA(TBaseStructure<T>::Get());
}

template<typename T>
UE_NODISCARD FORCEINLINE bool FAnyStruct::IsAExact() const
{
	return IsAExact(TBaseStructure<T>::Get());
}

template<typename T>
UE_NODISCARD FORCEINLINE T* FAnyStruct::Get()
{
	return IsA<T>() ? (T*)RawMemory : nullptr;
}

template<typename T>
UE_NODISCARD FORCEINLINE const T* FAnyStruct::Get() const
{
	return IsA<T>() ? (T*)RawMemory : nullptr;
}

template<typename T>
UE_NODISCARD FORCEINLINE T& FAnyStruct::GetChecked()
{
	checkf(IsA<T>(), TEXT("AnyStruct could not be casted to %s"), *TBaseStructure<T>::Get()->GetName());
	return *(T*)RawMemory;
}

template<typename T>
UE_NODISCARD FORCEINLINE const T& FAnyStruct::GetChecked() const
{
	checkf(IsA<T>(), TEXT("AnyStruct could not be casted to %s"), *TBaseStructure<T>::Get()->GetName());
	return *(T*)RawMemory;
}

inline void FAnyStruct::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	if (!IsValid()) return;

	Collector.AddReferencedObject(Type);

	if (Type->StructFlags & STRUCT_AddStructReferencedObjects)
	{
		Type->GetCppStructOps()->AddStructReferencedObjects()(RawMemory, Collector);
	}

	// Iterate through all properties (even within nested structs) for object properties
	for (TPropertyValueIterator<FObjectProperty> It(Type, RawMemory); It; ++It)
	{
		UObject*& ReferencedObject = *(UObject**)It.Value();
		Collector.AddReferencedObject(ReferencedObject);
	}
}

inline bool FAnyStruct::Serialize(FArchive& Ar)
{
	const UScriptStruct* NewType;
	if (Ar.IsSaving())
		NewType = Type;

	Ar << (UScriptStruct*&)NewType;

	if (NewType)
	{
		// Initialize struct
		if (Ar.IsLoading())
		{
			Set(NewType);
		}

		Type->SerializeBin(Ar, RawMemory);
	}
	else if (Ar.IsLoading())
	{
		Destroy();
	}

	return true;
}


inline void NetSerializeStructProps(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess, const UScriptStruct* Type, uint8* RawMemory)
{
	// Use native net-serializer - if one exists
	UScriptStruct::ICppStructOps* StructOps = Type->GetCppStructOps();
	if (StructOps && StructOps->HasNetSerializer())
	{
		StructOps->NetSerialize(Ar, Map, bOutSuccess, RawMemory);
		return;
	}

	// Recursively iterate over each struct member property and serialize
	for (TFieldIterator<FProperty> It(Type); It; ++It)
	{
		if (It->PropertyFlags & CPF_RepSkip) continue;// Skip this property
		if (const FStructProperty* StructProp = CastField<FStructProperty>(*It))// Recursively net serialize struct property
		{
				NetSerializeStructProps(Ar, Map, bOutSuccess, StructProp->Struct, StructProp->ContainerPtrToValuePtr<uint8>(RawMemory));
		}
		else if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(*It))// Net serialize each array element
		{
			FScriptArrayHelper_InContainer Arr(ArrProp, RawMemory);

			uint32 Num = Ar.IsSaving() ? Arr.Num() : 0;
			Ar.SerializeBits(&Num, 31);// 32nd bit is unused

			if (Ar.IsLoading())
			{
				Arr.Resize(Num);
			}

			if (ArrProp->Inner->PropertyFlags & CPF_RepSkip) continue;
			const FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrProp->Inner);
			
			for (uint32 i = 0; i < Num; i++)
			{
				if (InnerStructProp)
				{
					NetSerializeStructProps(Ar, Map, bOutSuccess, InnerStructProp->Struct, Arr.GetRawPtr(i));
				}
				else
				{
					ArrProp->Inner->NetSerializeItem(Ar, Map, Arr.GetRawPtr(i));
				}
			}
		}
		else// Basic property serialize
		{
			It->NetSerializeItem(Ar, Map, It->ContainerPtrToValuePtr<uint8>(RawMemory));
		}
	}
}

inline bool FAnyStruct::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bSerializingAny = false;
	if (Ar.IsSaving())
		bSerializingAny = IsValid();

	Ar.SerializeBits(&bSerializingAny, 1);

	if (!bSerializingAny)
	{
		if (Ar.IsLoading())
			Destroy();

		return true;
	}

	const UScriptStruct* NewType;
	if (Ar.IsSaving())
		NewType = Type;

	Ar << (UScriptStruct*&)NewType;

	check(NewType != nullptr);

	if (Ar.IsLoading() && Type != NewType)
	{
		Set(NewType);
	}

	NetSerializeStructProps(Ar, Map, bOutSuccess, Type, RawMemory);

	return true;
}