
#pragma once

#include "CoreMinimal.h"
#include "ECSIDs.h"
#include "ECSBaseTypes.h"
#include "ECSTypeDescriptions.generated.h"

struct FArchetypeEntityRecord
{
	FArchetypeEntityRecord() = delete;
	FORCEINLINE explicit constexpr FArchetypeEntityRecord(const FArchetypeID& ArchetypeID, const int32 ColumnIndex)
		: ArchetypeID(ArchetypeID), ColumnIndex(ColumnIndex) {}

	FArchetypeID ArchetypeID;
	int32 ColumnIndex;
};

struct FArchetypeCompRecord
{
	FArchetypeCompRecord() = delete;
	FORCEINLINE explicit constexpr FArchetypeCompRecord(const FArchetypeID& ID, const int32 RowIndex)
		: ID(ID), RowIndex(RowIndex) {}

	FArchetypeID ID;
	int32 RowIndex;
};

USTRUCT()
struct ECSUTILS_API FCompDescription
{
	GENERATED_BODY()

	FCompDescription() = delete;
	FORCEINLINE explicit FCompDescription(EForceInit) : Type(nullptr) {}
	FORCEINLINE explicit FCompDescription(const UScriptStruct* Type)
		: Type(Type)
	{
		check(Type);
		check(Type->IsChildOf(FECSCompBase::StaticStruct()));
	}

	UPROPERTY()
	const UScriptStruct* Type;

	TArray<FArchetypeCompRecord> ReferencedArchetypes;
};

template<>
struct TStructOpsTypeTraits<FCompDescription> : TStructOpsTypeTraitsBase2<FCompDescription>
{
	enum
	{
		WithNoInitConstructor = true,
	};
};

USTRUCT()
struct ECSUTILS_API FTagDescription
{
	GENERATED_BODY()

	FTagDescription() = delete;
	FORCEINLINE explicit FTagDescription(EForceInit) : Type(nullptr) {}
	FORCEINLINE explicit FTagDescription(const UScriptStruct* Type)
		: Type(Type)
	{
		check(Type);
		check(Type->IsChildOf(FECSTagBase::StaticStruct()));
	}

	UPROPERTY()
	const UScriptStruct* Type;

	TArray<FArchetypeCompRecord> ReferencedArchetypes;
};

template<>
struct TStructOpsTypeTraits<FTagDescription> : TStructOpsTypeTraitsBase2<FTagDescription>
{
	enum
	{
		WithNoInitConstructor = true,
	};
};