
#include "ECSSubsystem.h"

#include "Algo/Accumulate.h"
#include "Types/AnyStructArray.h"
#include "Types/Archetype.h"
#include "Types/CompQuery.h"

#define PRINT(Fmt, ...) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Purple, FString::Printf(TEXT(Fmt), ##__VA_ARGS__));

UECSSubsystem::UECSSubsystem()
{
   
}

template<typename T>
FString ToString(const FAnyStructArray& Arr)
{
	FString OutStr = FString::Printf(TEXT("%s:\n{\n"), Arr.GetType() ? *Arr.GetType()->GetName() : TEXT("NULL"));
	for (int32 i = 0; i < Arr.Num(); i++)
	{
		OutStr += FString::Printf(TEXT("    [%i]: %s"), i, *Arr.Get<T>(i).ToString());
		if (i != Arr.Num() - 1)
		{
			OutStr += TEXT(",");
		}

		OutStr += TEXT("\n");
	}
	return OutStr += TEXT("}");
}

void UECSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	RegisterComponentsAndTags();
	
	Super::Initialize(Collection);

	// Testing stuff
	const FEntityID NewEntity = SpawnEntity<TCompTypes<FComp1, FComp2>>(FComp1(), FVector(999.f));
	const FEntityID OtherNewEntity = SpawnEntity<TCompTypes<FComp2, FComp1>>(FVector(93802.f), FComp1());
	const FEntityID OtherOtherNewEntity = SpawnEntity<TCompTypes<FComp1, FComp3>>();

	using FQuery = TCompQuery<TReads<FComp2>>;
	const FQuery Query(this);
	
	Query.ForEach([](const FComp2& Comp)
	{
		PRINT("Comp == %s!", *Comp.Something.ToString());
	});
}

void UECSSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UECSSubsystem::RegisterComponentsAndTags()
{
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		if (*It == FECSCompBase::StaticStruct() || *It == FECSTagBase::StaticStruct()) continue;
		if (It->IsChildOf(FECSCompBase::StaticStruct()))
		{
			RegisteredComponents.Emplace(*It);
		}
		else if (It->IsChildOf(FECSTagBase::StaticStruct()))
		{
			RegisteredTags.Emplace(*It);
		}
	}

	RegisteredComponents.Sort([](const FCompDescription& A, const FCompDescription& B)->bool
		{
			const int32 SizeA = A.Type->GetStructureSize(), SizeB = B.Type->GetStructureSize();
			return SizeA != SizeB ? SizeA < SizeB : A.Type->GetUniqueID() < B.Type->GetUniqueID();
		});

	RegisteredTags.Sort([](const FTagDescription& A, const FTagDescription& B)->bool
		{
			return A.Type->GetUniqueID() < B.Type->GetUniqueID();
		});
}

UE_NODISCARD FCompTypeID UECSSubsystem::FindCompTypeID(const UScriptStruct* Type) const
{
	checkf(!RegisteredComponents.IsEmpty(), TEXT("Attempted to retrieve a component type ID before any components have been registered!"));
	check(Type);
	check(Type->IsChildOf(FECSCompBase::StaticStruct()));
	return FCompTypeID(Algo::BinarySearchBy(RegisteredComponents, Type, &FCompDescription::Type, [](const UScriptStruct* A, const UScriptStruct* B)->bool
	{
		const int32 SizeA = A->GetStructureSize(), SizeB = B->GetStructureSize();
		return SizeA != SizeB ? SizeA < SizeB : A->GetUniqueID() < B->GetUniqueID();
	}));
}

UE_NODISCARD FTagTypeID UECSSubsystem::FindTagTypeID(const UScriptStruct* Type) const
{
	checkf(!RegisteredTags.IsEmpty(), TEXT("Attempted to retrieve a tag type ID before any components have been registered!"));
	check(Type);
	check(Type->IsChildOf(FECSTagBase::StaticStruct()));
	return FTagTypeID(Algo::BinarySearchBy(RegisteredTags, Type, &FTagDescription::Type, [](const UScriptStruct* A, const UScriptStruct* B)->bool
		{
			return A->GetUniqueID() < B->GetUniqueID();
		}));
}

UE_NODISCARD FArchetypeID UECSSubsystem::FindArchetypeID(const TConstArrayView<FCompTypeID>& CompIDs, const TConstArrayView<FTagTypeID>& TagIDs) const
{
	checkf(!RegisteredComponents.IsEmpty(), TEXT("Attempted to retrieve a archetype ID before any components have been registered!"));
	checkf(!RegisteredTags.IsEmpty(), TEXT("Attempted to retrieve a archetype ID before any components have been registered!"));
	check(!CompIDs.IsEmpty());
	
	if (TagIDs.IsEmpty())
	{
		const int32 NumWords = FMath::DivideAndRoundUp(RegisteredComponents.Num(), 8);
		uint8* CompIDsBitMask = (uint8*)FMemory_Alloca(NumWords);
		FMemory::Memset(CompIDsBitMask, 0, NumWords);
		for (const FCompTypeID& ID : CompIDs)
			CompIDsBitMask[(int32)ID / 8] |= (1 << (int32)ID % 8);
	}

	return FArchetypeID(INDEX_NONE);
}
