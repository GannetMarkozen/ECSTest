
#pragma once

#include "CoreMinimal.h"
#include "AnyStructArray.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ECSUTILS_API FAnyStructArray
{
	GENERATED_BODY()

	constexpr FAnyStructArray();
	explicit FAnyStructArray(ENoInit);
	explicit FAnyStructArray(const UScriptStruct* ScriptStruct);
	explicit FAnyStructArray(const UScriptStruct* ScriptStruct, const void* Copy, const int32 ArrNum = 1);
	FAnyStructArray(const FAnyStructArray& Other);
	FAnyStructArray(FAnyStructArray&& Other) noexcept;
	template<typename T> FAnyStructArray(const std::initializer_list<T>& InitList);
	~FAnyStructArray();

	template<typename T>
	static FAnyStructArray Make();

	template<typename T>
	static FAnyStructArray Make(const int32 Num);

	template<typename T, typename SizeType = int32>
	static FAnyStructArray Make(const TConstArrayView<T, SizeType>& ArrayView);

	FAnyStructArray& operator=(const FAnyStructArray& Other);
	FAnyStructArray& operator=(FAnyStructArray&& Other) noexcept;
	template<typename T> FAnyStructArray& operator=(const std::initializer_list<T>& InitList);

	bool operator==(const FAnyStructArray& Other) const;
	bool operator!=(const FAnyStructArray& Other) const;

	void Empty();

	template<typename T, typename... ParamTypes>
	int32 Emplace(ParamTypes&&... Params);

	template<typename T, typename... ParamTypes>
	T& Emplace_GetRef(ParamTypes&&... Params);

	template<typename T>
	int32 Add(T&& Other);

	template<typename T>
	T& Add_GetRef(T&& Other);

	template<typename T>
	T& Insert(T&& Value, const int32 Index);

	template<typename T, typename SizeType = int32>
	void Append(const TConstArrayView<T, SizeType>& ArrayView);

	template<typename T>
	void Append(const std::initializer_list<T>& InitList);

	int32 AddDefaulted(const int32 Num);
	int32 AddUninitialized(const int32 Num);

	int32 AddFromBuffer(const void* CopyValue);

	void AppendFromBuffer(const void* CopyValues, const int32 Num);

	void* InsertAtFromBuffer(const void* CopyValue, const int32 Index);

	void SetNumUninitialized(const int32 Num);
	void SetNum(const int32 Num);

	void RemoveAt(const int32 Index, const int32 Num = 1);

	// Resizes array without calling constructors / destructors. VERY DANGEROUS!
	void Resize(const int32 NewSize);

	template<typename T>
	T& Get(const int32 Index);

	template<typename T>
	const T& Get(const int32 Index) const;

	void* GetRawPtr(const int32 Index);
	const void* GetRawPtr(const int32 Index) const;

	template<typename T>
	T& Last();

	template<typename T>
	const T& Last() const;
	
	void* Last();
	const void* Last() const;
	
	bool IsEmpty() const;
	bool IsValidIndex(const int32 Index) const;

	bool IsA(const UScriptStruct* InType) const;

	template<typename T>
	bool IsA() const;

	int32 Num() const;
	int32 GetStructureSize() const;
	int32 GetAlignment() const;

	//~ Type traits
	void AddStructReferencedObjects(FReferenceCollector& Collector);
	//~

	//~ Simple getters
	FORCEINLINE const void* GetAllocation() const { return Memory; }
	FORCEINLINE const UScriptStruct* GetType() const { return ScriptStruct; }
	//~
	
	//~ Iterator
	template<typename T> class TIterator;
	template<typename T> class TConstIterator;

	template<typename T>
	TIterator<T> ForEach();

	template<typename T>
	TConstIterator<T> ForEach() const;
	//~

protected:
	uint8* Memory;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, DisplayName="Type")
	const UScriptStruct* ScriptStruct;
	
	int32 NumElems;
};

template<>
struct TStructOpsTypeTraits<FAnyStructArray> : TStructOpsTypeTraitsBase2<FAnyStructArray>
{
	enum
	{
		WithCopy = true,
		WithIdenticalViaEquality = true,
		WithAddStructReferencedObjects = true,
	};
};

FORCEINLINE constexpr FAnyStructArray::FAnyStructArray()
	: Memory(nullptr), ScriptStruct(nullptr), NumElems(0) {}

FORCEINLINE FAnyStructArray::FAnyStructArray(ENoInit) {}

FORCEINLINE FAnyStructArray::FAnyStructArray(const UScriptStruct* ScriptStruct)
	: Memory(nullptr), ScriptStruct(ScriptStruct), NumElems(0) { check(ScriptStruct != nullptr); }

template<typename T>
FORCEINLINE FAnyStructArray::FAnyStructArray(const std::initializer_list<T>& InitList)
	: FAnyStructArray(FAnyStructArray::Make<T, int32>(InitList)) {}

inline FAnyStructArray::FAnyStructArray(const UScriptStruct* ScriptStruct, const void* Copy, const int32 ArrNum)
	: ScriptStruct(ScriptStruct), NumElems(ArrNum)
{
	check(ScriptStruct);
	check(NumElems > INDEX_NONE);
	check(Copy);
	if (NumElems == 0) return;

	Memory = (uint8*)FMemory::Malloc(NumElems * GetStructureSize(), GetAlignment());
	ScriptStruct->InitializeStruct(Memory, NumElems);
	ScriptStruct->CopyScriptStruct(Memory, Copy, NumElems);
}

inline FAnyStructArray::FAnyStructArray(const FAnyStructArray& Other)
	: ScriptStruct(Other.ScriptStruct), NumElems(Other.NumElems)
{
	if (Other.IsEmpty())
	{
		Memory = nullptr;
		return;
	}

	check(ScriptStruct != nullptr);
	Memory = (uint8*)FMemory::Malloc(NumElems * ScriptStruct->GetStructureSize(), ScriptStruct->GetMinAlignment());
	ScriptStruct->InitializeStruct(Memory, NumElems);
	ScriptStruct->CopyScriptStruct(Memory, Other.Memory, NumElems);
}

FORCEINLINE FAnyStructArray::FAnyStructArray(FAnyStructArray&& Other) noexcept
	: Memory(Other.Memory), ScriptStruct(Other.ScriptStruct), NumElems(Other.NumElems)
{
	Other.Memory = nullptr;
	Other.ScriptStruct = nullptr;
	Other.NumElems = 0;
}

FORCEINLINE FAnyStructArray::~FAnyStructArray()
{
	if (IsEmpty()) return;

	ScriptStruct->DestroyStruct(Memory, NumElems);
	FMemory::Free(Memory);
}

template<typename T>
FORCEINLINE FAnyStructArray FAnyStructArray::Make()
{
	FAnyStructArray Out(NoInit);
	Out.Memory = nullptr;
	Out.ScriptStruct = TBaseStructure<T>::Get();
	Out.NumElems = 0;
	return Out;
}

template<typename T>
inline FAnyStructArray FAnyStructArray::Make(const int32 Num)
{
	FAnyStructArray Out(NoInit);
	Out.ScriptStruct = TBaseStructure<T>::Get();
	Out.NumElems = FMath::Max(Num, 0);

	Out.Memory = (uint8*)FMemory::Malloc(Num * sizeof(T), alignof(T));
	for (int32 i = 0; i < Out.NumElems; ++i)
	{
		if constexpr (TStructOpsTypeTraits<T>::WithNoInitConstructor)
		{
			new (Out.Memory + i * sizeof(T)) T(ForceInit);
		}
		else
		{
			new (Out.Memory + i * sizeof(T)) T();
		}
	}

	return Out;
}

template<typename T, typename SizeType>
inline FAnyStructArray FAnyStructArray::Make(const TConstArrayView<T, SizeType>& ArrayView)
{
	FAnyStructArray Out(NoInit);
	Out.ScriptStruct = TBaseStructure<T>::Get();
	Out.NumElems = ArrayView.Num();
	if (Out.NumElems == 0)
	{
		Out.Memory = nullptr;
		return Out;
	}
	
	Out.Memory = (uint8*)FMemory::Malloc(ArrayView.Num() * sizeof(T), alignof(T));
	for (int32 i = 0; i < Out.NumElems; ++i)
	{
		new (Out.Memory + i * sizeof(T)) T{ArrayView[i]};
	}
	
	return Out;
}

FORCEINLINE FAnyStructArray& FAnyStructArray::operator=(const FAnyStructArray& Other)
{
	Empty();
	return *new (this) FAnyStructArray{Other};
}

FORCEINLINE FAnyStructArray& FAnyStructArray::operator=(FAnyStructArray&& Other) noexcept
{
	Empty();
	return *new (this) FAnyStructArray{MoveTemp(Other)};
}

template<typename T>
FORCEINLINE FAnyStructArray& FAnyStructArray::operator=(const std::initializer_list<T>& InitList)
{
	Empty();
	return *new (this) FAnyStructArray{InitList};
}

UE_NODISCARD inline bool FAnyStructArray::operator==(const FAnyStructArray& Other) const
{
	if (NumElems != Other.NumElems) return false;
	if (IsEmpty()) return true;
	if (ScriptStruct != Other.ScriptStruct) return false;

	const int32 StructureSize = GetStructureSize();
	for (int32 Offset = 0; Offset < NumElems * StructureSize; Offset += StructureSize)
		if (!ScriptStruct->CompareScriptStruct(Memory + Offset, Other.Memory + Offset, EPropertyPortFlags::PPF_None))
			return false;

	return true;
}

UE_NODISCARD FORCEINLINE bool FAnyStructArray::operator!=(const FAnyStructArray& Other) const
{
	return !(*this == Other);
}

inline void FAnyStructArray::Empty()
{
	if (IsEmpty()) return;

	ScriptStruct->DestroyStruct(Memory, NumElems);
	FMemory::Free(Memory);
	Memory = nullptr;
	NumElems = 0;
}

template<typename T, typename... ParamTypes>
inline int32 FAnyStructArray::Emplace(ParamTypes&&... Params)
{
	checkf(ScriptStruct, TEXT("FAnyStructArray: Type is uninitialized!"));
	checkf(ScriptStruct == TBaseStructure<T>::Get(), TEXT("FAnyStructArray: Type mismatch!"));

	const int32 Index = NumElems++;
	if (!Memory)
	{
		check(NumElems == 1);
		Memory = (uint8*)FMemory::Malloc(sizeof(T), alignof(T));
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * sizeof(T), alignof(T));
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	new (Memory + Index * sizeof(T)) T{Forward<ParamTypes>(Params)...};
	return Index;
}

template<typename T, typename... ParamTypes>
inline T& FAnyStructArray::Emplace_GetRef(ParamTypes&&... Params)
{
	checkf(ScriptStruct, TEXT("FAnyStructArray: Type is uninitialized!"));
	checkf(ScriptStruct == TBaseStructure<T>::Get(), TEXT("FAnyStructArray: Type mismatch!"));

	const int32 Index = NumElems++;
	if (!Memory)
	{
		check(NumElems == 1);
		Memory = (uint8*)FMemory::Malloc(sizeof(T), alignof(T));
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * sizeof(T), alignof(T));
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	return *new (Memory + Index * sizeof(T)) T{Forward<ParamTypes>(Params)...};
}

template<typename T>
FORCEINLINE int32 FAnyStructArray::Add(T&& Other)
{
	return Emplace<T>(Forward<T>(Other));
}

template<typename T>
FORCEINLINE T& FAnyStructArray::Add_GetRef(T&& Other)
{
	return Emplace_GetRef<T>(Forward<T>(Other));
}

template<typename T, typename SizeType>
inline void FAnyStructArray::Append(const TConstArrayView<T, SizeType>& ArrayView)
{
	checkf(ScriptStruct, TEXT("FAnyStructArray: Type is uninitialized!"));
	checkf(ScriptStruct == TBaseStructure<T>::Get(), TEXT("FAnyStructArray: Type mismatch!"));
	if (ArrayView.Num() == 0) return;

	const int32 Index = NumElems;
	NumElems += ArrayView.Num();
	if (!Memory)
	{
		check(NumElems > 0);
		Memory = (uint8*)FMemory::Malloc(NumElems * sizeof(T), alignof(T));
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * sizeof(T), alignof(T));
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	for (int32 i = Index; i < NumElems; i++)
	{
		new (Memory + i * sizeof(T)) T{ArrayView[i]};
	}
}

template<typename T>
FORCEINLINE void FAnyStructArray::Append(const std::initializer_list<T>& InitList)
{
	Append<T, int32>(InitList);
}

template<typename T>
inline T& FAnyStructArray::Insert(T&& Value, const int32 Index)
{
	checkf(ScriptStruct, TEXT("FAnyStructArray: Type is uninitialized!"));
	checkf(ScriptStruct == TBaseStructure<T>::Get(), TEXT("FAnyStructArray: Type mismatch!"));
	check(Index <= NumElems && Index > INDEX_NONE);

	NumElems += 1;
	if (!Memory)
	{
		check(NumElems == 1);
		Memory = (uint8*)FMemory::Malloc(sizeof(T), alignof(T));
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * sizeof(T), alignof(T));
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	uint8* InsertItem = Memory + Index * sizeof(T);
	const int32 MoveNum = NumElems - (Index + 1);
	if (MoveNum != 0)
	{
		FMemory::Memmove(InsertItem + sizeof(T), InsertItem, MoveNum * sizeof(T));
	}

	return *new (InsertItem) T{Forward<T>(Value)};
}

FORCEINLINE int32 FAnyStructArray::AddDefaulted(const int32 Num)
{
	const int32 FirstIndex = AddUninitialized(Num);
	ScriptStruct->InitializeStruct(GetRawPtr(FirstIndex), Num);
	return FirstIndex;
}

inline int32 FAnyStructArray::AddUninitialized(const int32 Num)
{
	checkf(ScriptStruct, TEXT("FAnyStructArray: Type is uninitialized!"));
	check(Num > 0);

	const int32 Size = GetStructureSize();
	const int32 Alignment = GetAlignment();

	const int32 OldNumElems = NumElems;
	NumElems += Num;
	if (!Memory)
	{
		check(NumElems > 0);
		Memory = (uint8*)FMemory::Malloc(NumElems * Size, Alignment);
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * Size, Alignment);
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	return OldNumElems;
}

inline void* FAnyStructArray::InsertAtFromBuffer(const void* CopyValue, const int32 Index)
{
	checkf(ScriptStruct, TEXT("FAnyStructArray: Type is uninitialized!"));
	check(Index <= NumElems && Index > INDEX_NONE);

	const int32 Size = GetStructureSize();
	const int32 Alignment = GetAlignment();
	NumElems += 1;
	if (!Memory)
	{
		check(NumElems == 1);
		Memory = (uint8*)FMemory::Malloc(Size, Alignment);
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * Size, Alignment);
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	uint8* InsertItem = Memory + Index * Size;
	const int32 MoveNum = NumElems - (Index + 1);
	if (MoveNum != 0)
	{
		FMemory::Memmove(InsertItem + Size, InsertItem, MoveNum * Size);
	}

	ScriptStruct->InitializeStruct(InsertItem);
	ScriptStruct->CopyScriptStruct(InsertItem, CopyValue);
	return InsertItem;
}

inline int32 FAnyStructArray::AddFromBuffer(const void* CopyValue)
{
	check(CopyValue);
	checkf(ScriptStruct, TEXT("FAnyStructArray: Type is uninitialized!"));

	const int32 Index = NumElems++;
	const int32 Size = GetStructureSize();
	const int32 Alignment = GetAlignment();
	if (!Memory)
	{
		check(NumElems == 1);
		Memory = (uint8*)FMemory::Malloc(Size, Alignment);
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * Size, Alignment);
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	uint8* Item = Memory + Index * Size;
	ScriptStruct->InitializeStruct(Item);
	ScriptStruct->CopyScriptStruct(Item, CopyValue);

	return Index;
}

inline void FAnyStructArray::AppendFromBuffer(const void* CopyValues, const int32 Num)
{
	check(Num > INDEX_NONE);
	if (!CopyValues || Num == 0) return;

	const int32 Index = NumElems;
	NumElems += Num;
	const int32 Size = GetStructureSize();
	const int32 Alignment = GetAlignment();
	if (!Memory)
	{
		check(NumElems >= 1);
		Memory = (uint8*)FMemory::Malloc(NumElems * Size, Alignment);
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * Size, Alignment);
	}
	checkf(Memory, TEXT("FAnyStructArray: Memory allocation failed!"));

	uint8* FirstItem = Memory + Index * Size;
	ScriptStruct->InitializeStruct(FirstItem, Num);
	ScriptStruct->CopyScriptStruct(FirstItem, CopyValues, Num);
}

inline void FAnyStructArray::RemoveAt(const int32 Index, const int32 Num)
{
	check(IsValidIndex(Index));
	check(Num > 0);

	// Destroy the desired elements of the array
	const int32 RemoveNum = FMath::Min(Num, NumElems - Index);
	const int32 Size = GetStructureSize();
	uint8* RemoveItem = Memory + Index * Size;
	ScriptStruct->DestroyStruct(RemoveItem, RemoveNum);

	// Move all other elements down
	const int32 MoveNum = NumElems - (Index + RemoveNum);
	if (MoveNum != 0)
	{
		const uint8* MoveItem = Memory + (Index + RemoveNum) * Size;
		FMemory::Memmove(RemoveItem, MoveItem, MoveNum * Size);
	}

	// Resize memory to fit new element count
	NumElems -= RemoveNum;
	if (NumElems == 0)
	{
		FMemory::Free(Memory);
		Memory = nullptr;
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * Size, GetAlignment());
	}
}

template<typename T>
UE_NODISCARD FORCEINLINE T& FAnyStructArray::Get(const int32 Index)
{
	checkf(ScriptStruct == TBaseStructure<T>::Get(), TEXT("FAnyStructArray: Type mismatch!"));
	check(IsValidIndex(Index));
	return ((T*)Memory)[Index];
}

template<typename T>
UE_NODISCARD FORCEINLINE const T& FAnyStructArray::Get(const int32 Index) const
{
	return const_cast<FAnyStructArray*>(this)->Get<T>(Index);
}

UE_NODISCARD FORCEINLINE void* FAnyStructArray::GetRawPtr(const int32 Index)
{
	check(IsValidIndex(Index));
	return Memory + Index * GetStructureSize();
}

UE_NODISCARD FORCEINLINE const void* FAnyStructArray::GetRawPtr(const int32 Index) const
{
	check(IsValidIndex(Index));
	return Memory + Index * GetStructureSize();
}

template<typename T>
UE_NODISCARD FORCEINLINE T& FAnyStructArray::Last()
{
	checkf(!IsEmpty(), TEXT("FAnyStructArray: Attempted to retrieve last element from an empty array!"));
	return Get<T>(NumElems - 1);
}

template<typename T>
UE_NODISCARD FORCEINLINE const T& FAnyStructArray::Last() const
{
	checkf(!IsEmpty(), TEXT("FAnyStructArray: Attempted to retrieve last element from an empty array!"));
	return Get<T>(NumElems - 1);
}

UE_NODISCARD FORCEINLINE void* FAnyStructArray::Last()
{
	checkf(!IsEmpty(), TEXT("FAnyStructArray: Attempted to retrieve last element from an empty array!"));
	return GetRawPtr(NumElems - 1);
}

UE_NODISCARD FORCEINLINE const void* FAnyStructArray::Last() const
{
	checkf(!IsEmpty(), TEXT("FAnyStructArray: Attempted to retrieve last element from an empty array!"));
	return GetRawPtr(NumElems - 1);
}

inline void FAnyStructArray::SetNumUninitialized(const int32 Num)
{
	check(Num > INDEX_NONE);
	if (Num == NumElems) return;

	const int32 StructureSize = GetStructureSize();
	if (Num < NumElems)
	{
		ScriptStruct->DestroyStruct(Memory + (Num - 1) * StructureSize, NumElems - Num);
	}

	if (Num == 0)
	{
		FMemory::Free(Memory);
		Memory = nullptr;
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, Num * StructureSize, ScriptStruct->GetMinAlignment());
	}

	NumElems = Num;
}

inline void FAnyStructArray::SetNum(const int32 Num)
{
	check(Num > INDEX_NONE);
	if (Num == NumElems) return;

	const int32 StructureSize = GetStructureSize();
	if (Num < NumElems)
	{
		ScriptStruct->DestroyStruct(Memory + (Num - 1) * StructureSize, NumElems - Num);
	}
	else
	{
		ScriptStruct->InitializeStruct(Memory + (NumElems - 1) * StructureSize, Num - NumElems);
	}

	if (Num == 0)
	{
		FMemory::Free(Memory);
		Memory = nullptr;
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, Num * StructureSize, ScriptStruct->GetMinAlignment());
	}

	NumElems = Num;
}

inline void FAnyStructArray::Resize(const int32 NewSize)
{
	check(NewSize > INDEX_NONE);
	if (NumElems == NewSize) return;
	NumElems = NewSize;
	if (NumElems == 0)
	{
		FMemory::Free(Memory);
		Memory = nullptr;
	}
	else if (Memory)
	{
		Memory = (uint8*)FMemory::Malloc(NumElems * GetStructureSize(), GetAlignment());
	}
	else
	{
		Memory = (uint8*)FMemory::Realloc(Memory, NumElems * GetStructureSize(), GetAlignment());
	}
}

FORCEINLINE bool FAnyStructArray::IsEmpty() const
{
	return NumElems == 0;
}

FORCEINLINE bool FAnyStructArray::IsValidIndex(const int32 Index) const
{
	return Index < NumElems && Index > INDEX_NONE;
}

FORCEINLINE int32 FAnyStructArray::Num() const
{
	return NumElems;
}

FORCEINLINE bool FAnyStructArray::IsA(const UScriptStruct* InType) const
{
	return ensure(ScriptStruct) && ScriptStruct->IsChildOf(InType);
}

template<typename T>
FORCEINLINE bool FAnyStructArray::IsA() const
{
	return IsA(TBaseStructure<T>::Get());
}

FORCEINLINE int32 FAnyStructArray::GetStructureSize() const
{
	check(ScriptStruct != nullptr);
	return ScriptStruct->GetStructureSize();
}

FORCEINLINE int32 FAnyStructArray::GetAlignment() const
{
	check(ScriptStruct != nullptr);
	return ScriptStruct->GetMinAlignment();
}

inline void FAnyStructArray::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ScriptStruct);

	if (!ScriptStruct) return;
	if (ScriptStruct->StructFlags & STRUCT_AddStructReferencedObjects)
	{
		ScriptStruct->GetCppStructOps()->AddStructReferencedObjects()(Memory, Collector);
	}

	// Iterate through all properties (even within nested structs) for object properties
	const int32 StructureSize = GetStructureSize();
	for (int32 Offset = 0; Offset < NumElems * StructureSize; Offset += StructureSize)
	{
		for (TPropertyValueIterator<FObjectProperty> It(ScriptStruct, Memory + Offset); It; ++It)
		{
			UObject*& ReferencedObject = *(UObject**)It.Value();
			Collector.AddReferencedObject(ReferencedObject);
		}
	}
}

//~ Iterator
template<typename T>
class FAnyStructArray::TIterator
{
	friend FAnyStructArray;
	TIterator() = delete;
	FORCEINLINE explicit TIterator(FAnyStructArray& Arr)
		: Arr(Arr)
	{
		checkf(Arr.GetType() == TBaseStructure<T>::Get(), TEXT("FAnyStructArray: Type mismatch!"));
	}
public:
	UE_NODISCARD FORCEINLINE T* begin() const { return (T*)Arr.Memory; }
	UE_NODISCARD FORCEINLINE T* end() const { return (T*)Arr.Memory + Arr.NumElems; }

private:
	FAnyStructArray& Arr;
};

template<typename T>
class FAnyStructArray::TConstIterator
{
	friend FAnyStructArray;
	TConstIterator() = delete;
	FORCEINLINE explicit TConstIterator(const FAnyStructArray& Arr)
		: Arr(Arr)
	{
		checkf(Arr.GetType() == TBaseStructure<T>::Get(), TEXT("FAnyStructArray: Type mismatch!"));
	}
public:
	UE_NODISCARD FORCEINLINE const T* begin() const { return (T*)Arr.Memory; }
	UE_NODISCARD FORCEINLINE const T* end() const { return (T*)Arr.Memory + Arr.NumElems; }

private:
	const FAnyStructArray& Arr;
};

template<typename T>
UE_NODISCARD FORCEINLINE FAnyStructArray::TIterator<T> FAnyStructArray::ForEach()
{
	return FAnyStructArray::TIterator<T>(*this);
}

template<typename T>
UE_NODISCARD FORCEINLINE FAnyStructArray::TConstIterator<T> FAnyStructArray::ForEach() const
{
	return FAnyStructArray::TConstIterator<T>(*this);
}
//