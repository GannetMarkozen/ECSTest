
#pragma once

#include "CoreMinimal.h"
#include "Algo/IndexOf.h"
#include "Types/AnyStructArray.h"
#include "Types/Archetype.h"
#include "Types/ECSBaseTypes.h"
#include "Types/ECSIDs.h"
#include "Types/ECSTypeDescriptions.h"
#include "Utilities/Metaprogramming.h"
#include "ECSSubsystem.generated.h"

UCLASS()
class ECSUTILS_API UECSSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UECSSubsystem();

	//~
	// Spawn entity

	// Spawn an entity with a given components / tags signature and call it's component's default constructor.
	template<typename InTCompTypes, typename InTTagTypes = TTagTypes<>>
	typename TEnableIf<TIsTCompTypes<InTCompTypes>::Value && TIsTTagTypes<InTTagTypes>::Value, FEntityID>::Type SpawnEntity();

	// Spawn an entity with a given components / tags signature and call it's component's constructors. Returns an ID for the new entity
	template<typename InTCompTypes, typename InTTagTypes = TTagTypes<>, typename... ParamTypes>
	typename TEnableIf<TIsTCompTypes<InTCompTypes>::Value && TIsTTagTypes<InTTagTypes>::Value && GetTypeListNum(InTCompTypes{}) == sizeof...(ParamTypes), FEntityID>::Type SpawnEntity(ParamTypes&&... Params);

	template<typename T>
	typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T*>::Type GetEntityComp(const FEntityID EntityID) const;

	template<typename T>
	typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T&>::Type GetEntityCompChecked(const FEntityID EntityID) const;

	void DestroyEntity(const FEntityID EntityID);

	bool IsValidEntity(const FEntityID EntityID) const;
	bool EntityHasComp(const FEntityID EntityID, const FCompTypeID CompTypeID) const;
	bool EntityHasTag(const FEntityID EntityID, const FTagTypeID TagTypeID) const;
	//~

	//~
	// ID Getters
	FCompTypeID FindCompTypeID(const UScriptStruct* Type) const;
	FTagTypeID FindTagTypeID(const UScriptStruct* Type) const;

	template<typename T>
	typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, FCompTypeID>::Type GetCompTypeID() const;

	template<typename T>
	typename TEnableIf<TIsDerivedFrom<T, FECSTagBase>::Value, FTagTypeID>::Type GetTagTypeID() const;

	const FCompDescription& GetCompDescription(const FCompTypeID ID) const;
	const FTagDescription& GetTagDescription(const FTagTypeID ID) const;

	template<typename T>
	typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, const FCompDescription&>::Type GetCompDescription() const;

	template<typename T>
	typename TEnableIf<TIsDerivedFrom<T, FECSTagBase>::Value, const FTagDescription&>::Type GetTagDescription() const;

	FArchetypeID FindArchetypeID(const TConstArrayView<FCompTypeID>& CompIDs, const TConstArrayView<FTagTypeID>& TagIDs) const;

	template<typename InTCompTypes, typename InTTagTypes = TTagTypes<>>
	typename TEnableIf<TIsTCompTypes<InTCompTypes>::Value && TIsTTagTypes<InTTagTypes>::Value, FArchetypeID>::Type GetArchetypeID() const;
	//~

	//~
	//Data getters
	const FArchetypeEntityRecord& GetEntityRecord(const FEntityID EntityID) const;

	FArchetype& GetArchetype(const FArchetypeID ArchetypeID) const;

	FORCEINLINE const TArray<FArchetype>& GetArchetypes() const { return RegisteredArchetypes; }
	FORCEINLINE int32 GetNumComps() const { return RegisteredComponents.Num(); }
	FORCEINLINE int32 GetNumTags() const { return RegisteredTags.Num(); }
	//~

protected:
	// Index via FEntityID
	using FEntityRecordSparseArray = TSparseArray<FArchetypeEntityRecord, TSparseArrayAllocator<TSizedDefaultAllocator<64>, TSizedDefaultAllocator<64>>>;
	FEntityRecordSparseArray EntityRecords;
	
	//~
	// Registered types
	UPROPERTY() mutable TArray<FCompDescription> RegisteredComponents;// Index via FCompTypeID
	UPROPERTY() mutable TArray<FTagDescription> RegisteredTags;// Index via FTagTypeID
	UPROPERTY() mutable TArray<FArchetype> RegisteredArchetypes;// Archetypes are lazily loaded. Index via FArchetypeID
	//~

private:
	// Number of entities to allocate at once when space runs out
	static constexpr SIZE_T ENTITY_ALLOC_CHUNK_SIZE = 64;
	
	void RegisterComponentsAndTags();

	template<typename... InTCompTypes, typename... InTTagTypes>
	FEntityID InternalSpawnEntity(TCompTypes<InTCompTypes...>&&, TTagTypes<InTTagTypes...>&&);

	template<typename... InTCompTypes, typename... InTTagTypes, typename... ParamTypes>
	FEntityID InternalSpawnEntityWithEmplace(TCompTypes<InTCompTypes...>&&, TTagTypes<InTTagTypes...>&&, ParamTypes&&... Params);
	
	template<typename... InTCompTypes, typename... InTTagTypes>
	FArchetypeID InternalFindArchetypeID(TCompTypes<InTCompTypes...>&&, TTagTypes<InTTagTypes...>&&) const;

	template<typename InTCompType, typename... OtherInTCompTypes, typename ParamType, typename... OtherParamTypes>
	void InternalConstructCompsAtColumn(TCompTypes<InTCompType, OtherInTCompTypes...>&&, FArchetype& Archetype, const int32 ColumnIndex, ParamType&& Param, OtherParamTypes&&... OtherParams);

protected:
	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface
};

/**
 * Impl
 */

template<typename InTCompTypes, typename InTTagTypes>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsTCompTypes<InTCompTypes>::Value && TIsTTagTypes<InTTagTypes>::Value, FEntityID>::Type UECSSubsystem::SpawnEntity()
{
	return InternalSpawnEntity(InTCompTypes{}, InTTagTypes{});
}

template<typename InTCompTypes, typename InTTagTypes, typename... ParamTypes>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsTCompTypes<InTCompTypes>::Value && TIsTTagTypes<InTTagTypes>::Value && GetTypeListNum(InTCompTypes{}) == sizeof...(ParamTypes), FEntityID>::Type UECSSubsystem::SpawnEntity(ParamTypes&&... Params)
{
	return InternalSpawnEntityWithEmplace(InTCompTypes{}, InTTagTypes{}, Forward<ParamTypes>(Params)...);
}

template<typename... InTCompTypes, typename... InTTagTypes>
UE_NODISCARD inline FEntityID UECSSubsystem::InternalSpawnEntity(TCompTypes<InTCompTypes...>&&, TTagTypes<InTTagTypes...>&&)
{
	const FArchetypeID ArchetypeID = GetArchetypeID<TCompTypes<InTCompTypes...>, TTagTypes<InTTagTypes...>>();
	check(RegisteredArchetypes.IsValidIndex((int32)ArchetypeID));
	
	FArchetype& Archetype = RegisteredArchetypes[(int32)ArchetypeID];
	const int32 ColumnIndex = Archetype.AddAtFirstUninitialized(nullptr, ENTITY_ALLOC_CHUNK_SIZE);// @TODO Make non-defaulted versions as well

	int32 Zero = 0;
	const int32 EntityIndex = EntityRecords.EmplaceAtLowestFreeIndex(Zero, ArchetypeID, ColumnIndex);
	return FEntityID(EntityIndex);
}

template<typename... InTCompTypes, typename... InTTagTypes, typename... ParamTypes>
UE_NODISCARD inline FEntityID UECSSubsystem::InternalSpawnEntityWithEmplace(TCompTypes<InTCompTypes...>&&, TTagTypes<InTTagTypes...>&&, ParamTypes&&... Params)
{
	const FArchetypeID ArchetypeID = GetArchetypeID<TCompTypes<InTCompTypes...>, TTagTypes<InTTagTypes...>>();
	check(RegisteredArchetypes.IsValidIndex((int32)ArchetypeID));
	
	FArchetype& Archetype = RegisteredArchetypes[(int32)ArchetypeID];
	
	// Find uninitialized column
	int32 ColumnIndex = Archetype.FindFirstUninitializedRow();

	if (UNLIKELY(ColumnIndex == INDEX_NONE))
	{
		ColumnIndex = Archetype.AddUninitialized(ENTITY_ALLOC_CHUNK_SIZE);
	}

	check(Archetype.IsValidColumn(ColumnIndex));
	checkf(!Archetype.IsColumnInitialized(ColumnIndex), TEXT("Attempted to initialize already initialized column at index %i"), ColumnIndex);
	
	InternalConstructCompsAtColumn(TCompTypes<InTCompTypes...>{}, Archetype, ColumnIndex, Forward<ParamTypes>(Params)...);
	Archetype.SetColumnInitializedFlag(true, ColumnIndex);

	int32 Zero = 0;
	const int32 EntityIndex = EntityRecords.EmplaceAtLowestFreeIndex(Zero, ArchetypeID, ColumnIndex);
	return FEntityID(EntityIndex);
}

template<typename InTCompType, typename... OtherInTCompTypes, typename ParamType, typename... OtherParamTypes>
FORCEINLINE void UECSSubsystem::InternalConstructCompsAtColumn(TCompTypes<InTCompType, OtherInTCompTypes...>&&, FArchetype& Archetype, const int32 ColumnIndex, ParamType&& Param, OtherParamTypes&&... OtherParams)
{
	// Calculate the component row index and call it's constructor there
	const int32 RowIndex = Archetype.GetCompRow(GetCompTypeID<InTCompType>());
	new (Archetype[RowIndex][ColumnIndex]) InTCompType{Forward<ParamType>(Param)};
	
	if constexpr (sizeof...(OtherInTCompTypes) != 0)
	{
		InternalConstructCompsAtColumn(TCompTypes<OtherInTCompTypes...>{}, Archetype, ColumnIndex, Forward<OtherParamTypes>(OtherParams)...);
	}
}

inline void UECSSubsystem::DestroyEntity(const FEntityID EntityID)
{
	check(IsValidEntity(EntityID));

	const FArchetypeEntityRecord& Record = EntityRecords[EntityID.ToInt()];
	check(RegisteredArchetypes.IsValidIndex(Record.ArchetypeID.ToInt()));

	FArchetype& Archetype = RegisteredArchetypes[Record.ArchetypeID.ToInt()];
	Archetype.DestructAt(Record.ColumnIndex);

	EntityRecords.RemoveAt(EntityID.ToInt());
}

FORCEINLINE bool UECSSubsystem::IsValidEntity(const FEntityID EntityID) const
{
	return EntityRecords.IsValidIndex(EntityID.ToInt());
}


inline bool UECSSubsystem::EntityHasComp(const FEntityID EntityID, const FCompTypeID CompTypeID) const
{
	if (!ensure(IsValidEntity(EntityID))) return false;
	
	const FArchetypeEntityRecord& Record = EntityRecords[EntityID.ToInt()];
	check(RegisteredArchetypes.IsValidIndex(Record.ArchetypeID.ToInt()));
	
	const int32 Index = CompTypeID.ToInt();
	return RegisteredArchetypes[Record.ArchetypeID.ToInt()].IncludedCompTagBitMask[Index / FArchetype::BITELEM_SIZE_BITS] & 1ull << Index % FArchetype::BITELEM_SIZE_BITS;
}

inline bool UECSSubsystem::EntityHasTag(const FEntityID EntityID, const FTagTypeID TagTypeID) const
{
	if (!ensure(IsValidEntity(EntityID))) return false;
	
	const FArchetypeEntityRecord& Record = EntityRecords[(int32)EntityID];
	check(RegisteredArchetypes.IsValidIndex((int32)Record.ArchetypeID));
	
	const int32 Index = TagTypeID.ToInt() + RegisteredComponents.Num();
	return RegisteredArchetypes[Record.ArchetypeID.ToInt()].IncludedCompTagBitMask[Index / FArchetype::BITELEM_SIZE_BITS] & 1ull << Index % FArchetype::BITELEM_SIZE_BITS;
}

template<typename T>
UE_NODISCARD inline typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T*>::Type UECSSubsystem::GetEntityComp(const FEntityID EntityID) const
{
	check(IsValidEntity(EntityID));

	const FCompTypeID CompID = GetCompTypeID<T>();
	if (!EntityHasComp(EntityID, CompID)) return nullptr;

	const FArchetypeEntityRecord& Record = GetEntityRecord(EntityID);
	const FArchetype& Archetype = RegisteredArchetypes[Record.ArchetypeID.ToInt()];
	
	const int32 RowIndex = Archetype.GetCompRow(CompID);

	check(Archetype[RowIndex].GetType() == TBaseStructure<T>::Get());
	return (T*)Archetype[RowIndex][Record.ColumnIndex];
}

template<typename T>
UE_NODISCARD inline typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T&>::Type UECSSubsystem::GetEntityCompChecked(const FEntityID EntityID) const
{
	check(IsValidEntity(EntityID));
	const FCompTypeID CompID = GetCompTypeID<T>();
	check(EntityHasComp(EntityID, CompID));

	const FArchetypeEntityRecord& Record = GetEntityRecord(EntityID);
	const FArchetype& Archetype = RegisteredArchetypes[(int32)Record.ArchetypeID];
	
	const int32 RowIndex = Archetype.GetCompRow(CompID);

	check(Archetype[RowIndex].GetType() == TBaseStructure<T>::Get());
	return *(T*)Archetype[RowIndex][Record.ColumnIndex];
}


template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, FCompTypeID>::Type UECSSubsystem::GetCompTypeID() const
{
	static const auto ID = FindCompTypeID(T::StaticStruct());
	return ID;
}

template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSTagBase>::Value, FTagTypeID>::Type UECSSubsystem::GetTagTypeID() const
{
	static const auto ID = FindTagTypeID(T::StaticStruct());
	return ID;
}

UE_NODISCARD FORCEINLINE const FCompDescription& UECSSubsystem::GetCompDescription(const FCompTypeID ID) const
{
	check(RegisteredComponents.IsValidIndex((int32)ID));
	return RegisteredComponents[(int32)ID];
}

UE_NODISCARD FORCEINLINE const FTagDescription& UECSSubsystem::GetTagDescription(const FTagTypeID ID) const
{
	check(RegisteredTags.IsValidIndex((int32)ID));
	return RegisteredTags[(int32)ID];
}

template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, const FCompDescription&>::Type UECSSubsystem::GetCompDescription() const
{
	return GetCompDescription(GetCompTypeID<T>());
}

template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSTagBase>::Value, const FTagDescription&>::Type UECSSubsystem::GetTagDescription() const
{
	return GetTagDescription(GetTagTypeID<T>());
}

template<typename InTCompTypes, typename InTTagTypes>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsTCompTypes<InTCompTypes>::Value && TIsTTagTypes<InTTagTypes>::Value, FArchetypeID>::Type UECSSubsystem::GetArchetypeID() const
{
	return InternalFindArchetypeID(InTCompTypes{}, InTTagTypes{});
}

template<typename... InTCompTypes, typename... InTTagTypes>
UE_NODISCARD inline FArchetypeID UECSSubsystem::InternalFindArchetypeID(TCompTypes<InTCompTypes...>&&, TTagTypes<InTTagTypes...>&&) const
{
	// Statically generate a bitmask from the template parameters for searching for a valid Archetype
	static const TBitArray<> BitMask([this]
		{
			TBitArray<> Bitmask(false, RegisteredComponents.Num() + RegisteredTags.Num());
			for (const FCompTypeID& ID : { GetCompTypeID<InTCompTypes>()... })
				Bitmask[(int32)ID] = true;

			if constexpr (sizeof...(InTTagTypes) != 0)
				for (const FTagTypeID& ID : { GetTagTypeID<InTTagTypes>()... })
					Bitmask[(int32)ID + RegisteredComponents.Num()] = true;
		
			return Bitmask;
		}());

	// @TODO: Improve linear search with a map or binary search
	// Find a matching archetype
	int32 Index = RegisteredArchetypes.IndexOfByPredicate([](const FArchetype& Archetype)->bool{ return FMemory::Memcmp(BitMask.GetData(), Archetype.IncludedCompTagBitMask, FMath::DivideAndRoundUp(Archetype.NumRows, 8)) == 0; });
	if (LIKELY(Index != INDEX_NONE))
		return FArchetypeID(Index);

	// If there was no matching archetype, generate a new archetype. Should only happen once per template instantiation
	const UScriptStruct* CompTypes[] = { InTCompTypes::StaticStruct()... };
	Algo::Sort(CompTypes, [](const UScriptStruct* A, const UScriptStruct* B)->bool
		{
			const int32 SizeA = A->GetStructureSize(), SizeB = B->GetStructureSize();
			return SizeA != SizeB ? SizeA < SizeB : A->GetUniqueID() < B->GetUniqueID();
		});

	Index = RegisteredArchetypes.Emplace(BitMask, TConstArrayView<const UScriptStruct*>(CompTypes, sizeof...(InTCompTypes)));
	const FArchetypeID NewID(Index);
	
	// Add newly generated archetype to component description's referenced archetypes array
	for (const FCompTypeID& CompTypeID : { GetCompTypeID<InTCompTypes>()... })
	{
		FCompDescription& CompDescription = RegisteredComponents[(int32)CompTypeID];
		TArray<FArchetypeCompRecord>& ArchetypeIDs = CompDescription.ReferencedArchetypes;
		ArchetypeIDs.EmplaceAt(Algo::LowerBoundBy(ArchetypeIDs, NewID, &FArchetypeCompRecord::ID), NewID, Algo::IndexOf(CompTypes, CompDescription.Type));
	}

	// Add newly generated archetype to tag description's referenced archetypes array
	if constexpr (sizeof...(InTTagTypes) != 0)
	{
		const UScriptStruct* TagTypes[] = { InTTagTypes::StaticStruct()... };
		Algo::Sort(TagTypes, [](const UScriptStruct* A, const UScriptStruct* B)->bool
			{
				return A->GetUniqueID() < B->GetUniqueID();
			});
		
		for (const FTagTypeID& TagTypeID : { GetTagTypeID<InTTagTypes>()... })
		{
			FTagDescription& TagDescription = RegisteredTags[(int32)TagTypeID];
			TArray<FArchetypeCompRecord>& ArchetypeIDs = TagDescription.ReferencedArchetypes;
			ArchetypeIDs.EmplaceAt(Algo::LowerBoundBy(ArchetypeIDs, NewID, &FArchetypeCompRecord::ID), NewID, Algo::IndexOf(TagTypes, TagDescription.Type));
		}
	}
	
	return NewID;
}

UE_NODISCARD FORCEINLINE const FArchetypeEntityRecord& UECSSubsystem::GetEntityRecord(const FEntityID EntityID) const
{
	check(IsValidEntity(EntityID));
	return EntityRecords[(int32)EntityID];
}

UE_NODISCARD FORCEINLINE FArchetype& UECSSubsystem::GetArchetype(const FArchetypeID ArchetypeID) const
{
	check(RegisteredArchetypes.IsValidIndex(ArchetypeID.ToInt()));
	return RegisteredArchetypes[ArchetypeID.ToInt()];
}

USTRUCT()
struct ECSUTILS_API FComp1 : public FECSCompBase
{
	GENERATED_BODY()

	FComp1() = default;
	FORCEINLINE explicit FComp1(const FTransform& Something) : Something(Something) {}

	FTransform Something;

	~FComp1()
	{
		UE_LOG(LogTemp, Warning, TEXT("Destructed FComp1!"));
	}
};

template<>
struct TStructOpsTypeTraits<FComp1> : TStructOpsTypeTraitsBase2<FComp1>
{
	enum
	{
		WithDestructor = true,
	};
};

USTRUCT()
struct ECSUTILS_API FComp2 : public FECSCompBase
{
	GENERATED_BODY()

	FComp2() = default;
	FORCEINLINE explicit FComp2(const FVector& Something) : Something(Something) {}

	FVector Something;
};

USTRUCT()
struct ECSUTILS_API FComp3 : public FECSCompBase
{
	GENERATED_BODY()

	uint8 Something;
};