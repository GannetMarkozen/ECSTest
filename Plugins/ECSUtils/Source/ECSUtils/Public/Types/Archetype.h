
#pragma once

#include "CoreMinimal.h"
#include "ECSBaseTypes.h"
#include "ECSIDs.h"
#include "Archetype.generated.h"

/**
 *
 */
USTRUCT()
struct ECSUTILS_API FArchetype
{
	GENERATED_BODY()
	struct FComponentsRow;
	friend class UECSSubsystem;
	
	FArchetype() = delete;
	explicit FArchetype(EForceInit);
	explicit FArchetype(const TBitArray<>& HasCompTagBitMask, const TConstArrayView<const UScriptStruct*>& Comps);
	~FArchetype();

	FComponentsRow& operator[](const int32 Index);
	const FComponentsRow& operator[](const int32 Index) const;

	bool IsValidRow(const int32 Index) const;
	bool IsValidColumn(const int32 Index) const;
	bool IsColumnInitialized(const int32 Index) const;

	bool HasSameSetIdentifierFlags(const FArchetype& Other, const int32 NumCompsAndTags) const;

	int32 GetCompRow(const FCompTypeID CompTypeID) const;
	int32 FindFirstUninitializedRow(const int32 StartColumn = 0) const;

	int32 AddAtFirstUninitialized(const TConstArrayView<const void*>* OptionalCopy = nullptr, const int32 AllocChunkIfNecessary = 1);
	
	int32 AddUninitialized(const int32 Num = 1);
	int32 AddDefaulted(const int32 Num = 1);
	bool DestructAt(const int32 ColumnIndex);

	//~
	// Getters
	template<typename T> typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T*>::Type Get(const int32 RowIndex, const int32 ColumnIndex);
	template<typename T> typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, const T*>::Type Get(const int32 RowIndex, const int32 ColumnIndex) const;
	template<typename T> typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T*>::Type Get(const int32 ColumnIndex); 
	template<typename T> typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, const T*>::Type Get(const int32 ColumnIndex) const;

	FORCEINLINE int32 GetNumRows() const { return NumRows; }
	FORCEINLINE int32 GetNumColumns() const { return NumColumns; }
	//~

	//~
	// Type traits
	void AddStructReferencedObjects(FReferenceCollector& Collector);
	//~

	//
	//~ Iterator
	FComponentsRow* begin();
	const FComponentsRow* begin() const;
	FComponentsRow* end();
	const FComponentsRow* end() const;
	//

	template<typename TFunctor>
	void ForEachInitializedColumn(TFunctor&& Functor) const;

	template<typename TFunctor>
	void ForEachInitializedColumnWithBreak(const int32 StartIndex, TFunctor&& Functor) const;
	
private:
	void SetColumnInitializedFlag(const bool bValue, const int32 Index);
	
	template<typename TFunctor>
	void ForEachRow(TFunctor&& Functor);

	template<typename TFunctor>
	void ForEachRow(TFunctor&& Functor) const;

#if PLATFORM_64BITS
	static constexpr SIZE_T BITELEM_SIZE_BYTES = 8;
#else
	static constexpr SIZE_T BITELEM_SIZE_BYTES = 4;
#endif
	
	static constexpr SIZE_T BITELEM_SIZE_BITS = BITELEM_SIZE_BYTES * 8;
	using FBitElem = TUnsignedIntType_T<BITELEM_SIZE_BYTES>;
	
	const int32 NumRows;
	int32 NumColumns;
	FBitElem* InitializedColumnBitMask;
	FBitElem* IncludedCompTagBitMask;
	FComponentsRow* Rows;
};

template<>
struct TStructOpsTypeTraits<FArchetype> : TStructOpsTypeTraitsBase2<FArchetype>
{
	enum
	{
		WithNoInitConstructor = true,
		WithCopy = false,
		WithAddStructReferencedObjects = true,
	};
};

struct FArchetype::FComponentsRow
{
	friend FArchetype;
	FComponentsRow() = delete;

	uint8* operator[](const int32 ColumnIndex);
	const uint8* operator[](const int32 ColumnIndex) const;

	FORCEINLINE const UScriptStruct* GetType() const { return ScriptStruct; }
	FORCEINLINE bool IsA(const UScriptStruct* Type) const { return ScriptStruct == Type; }
	template<typename T> FORCEINLINE bool IsA() const { return IsA(T::StaticStruct()); }

	template<typename T>
	UE_NODISCARD FORCEINLINE T& Get(const int32 Index) { check(ScriptStruct == TBaseStructure<T>::Get()); return ((T*)Memory)[Index]; }

	template<typename T>
	UE_NODISCARD FORCEINLINE const T& Get(const int32 Index) const { check(ScriptStruct == TBaseStructure<T>::Get()); return ((T*)Memory)[Index]; }

private:
	FORCEINLINE explicit FComponentsRow(const UScriptStruct* ScriptStruct)
		: Memory(nullptr), ScriptStruct(ScriptStruct)
	{
		check(ScriptStruct);
	}

	FORCEINLINE int32 GetSize() const { return ScriptStruct->GetStructureSize(); }
	FORCEINLINE int32 GetAlignment() const { return ScriptStruct->GetMinAlignment(); }

	uint8* Memory;
	const UScriptStruct* ScriptStruct;
};

FORCEINLINE uint8* FArchetype::FComponentsRow::operator[](const int32 ColumnIndex)
{
	check(Memory);
	return Memory + ColumnIndex * GetSize();
}

FORCEINLINE const uint8* FArchetype::FComponentsRow::operator[](const int32 ColumnIndex) const
{
	check(Memory);
	return Memory + ColumnIndex * GetSize();
}


/**
 * Impl
 */


FORCEINLINE FArchetype::FArchetype(EForceInit)
	: NumRows(0), NumColumns(0), InitializedColumnBitMask(nullptr), IncludedCompTagBitMask(nullptr), Rows(nullptr)
{
	check(false);
}

inline FArchetype::FArchetype(const TBitArray<>& HasCompTagBitMask, const TConstArrayView<const UScriptStruct*>& Comps)
	: NumRows(Comps.Num()), NumColumns(0), InitializedColumnBitMask(nullptr)
{
	// Allocate bitmask and copy
	const SIZE_T BitMaskNumBytes = FMath::DivideAndRoundUp<SIZE_T>(HasCompTagBitMask.Num(), BITELEM_SIZE_BITS) * BITELEM_SIZE_BYTES;
	IncludedCompTagBitMask = (FBitElem*)FMemory::Malloc(BitMaskNumBytes, alignof(FBitElem));
	FMemory::Memcpy(IncludedCompTagBitMask, HasCompTagBitMask.GetData(), BitMaskNumBytes);

	// Allocate rows and construct from comp types
	Rows = (FComponentsRow*)FMemory::Malloc(NumRows * sizeof(FComponentsRow), alignof(FComponentsRow));
	for (int32 i = 0; i < NumRows; i++)
		new (Rows + i) FComponentsRow(Comps[i]);
}

inline FArchetype::~FArchetype()
{
	// Destroy initialized components
	ForEachRow([this](FComponentsRow& Row)
	{
		ForEachInitializedColumn([&Row](const int32 Index)
		{
			Row.ScriptStruct->DestroyStruct(Row[Index]);
		});

		// Destruct row. May be unnecessary
		Row.~FComponentsRow();
	});

	// Free allocations
	FMemory::Free(Rows);
	FMemory::Free(InitializedColumnBitMask);
	FMemory::Free(IncludedCompTagBitMask);
}

FORCEINLINE bool FArchetype::IsValidRow(const int32 Index) const
{
	return Index < NumRows && Index > INDEX_NONE;
}

FORCEINLINE bool FArchetype::IsValidColumn(const int32 Index) const
{
	return Index < NumColumns && Index > INDEX_NONE;
}

FORCEINLINE bool FArchetype::IsColumnInitialized(const int32 Index) const
{
	check(IsValidColumn(Index));
	return InitializedColumnBitMask[Index / BITELEM_SIZE_BITS] & 1ull << Index % BITELEM_SIZE_BITS;
}

FORCEINLINE void FArchetype::SetColumnInitializedFlag(const bool bValue, const int32 Index)
{
	check(IsValidColumn(Index));
	if (bValue)
	{
		InitializedColumnBitMask[Index / BITELEM_SIZE_BITS] |= 1ull << Index % BITELEM_SIZE_BITS;
	}
	else
	{
		InitializedColumnBitMask[Index / BITELEM_SIZE_BITS] &= ~(1ull << Index % BITELEM_SIZE_BITS);
	}
}

inline int32 FArchetype::AddAtFirstUninitialized(const TConstArrayView<const void*>* OptionalCopy, const int32 AllocChunkIfNecessary)
{
	check(AllocChunkIfNecessary > 0);
	checkf(!OptionalCopy || OptionalCopy->Num() == NumRows, TEXT("Invalid number of columns"));
	
	int32 UninitializedRow = INDEX_NONE;
	for (int32 i = 0; i < NumColumns; ++i)
	{
		if (IsColumnInitialized(i)) continue;
		UninitializedRow = i;
		break;
	}

	if (UNLIKELY(UninitializedRow == INDEX_NONE))
	{
		UninitializedRow = AddUninitialized(AllocChunkIfNecessary);
		check(!IsColumnInitialized(UninitializedRow));
	}
	
	for (int32 i = 0; i < NumRows; ++i)
	{
		FComponentsRow& Column = Rows[i];
		uint8* Item = Column[UninitializedRow];
			
		Column.ScriptStruct->InitializeStruct(Item);
		if (OptionalCopy)
		{
			Column.ScriptStruct->CopyScriptStruct(Item, (*OptionalCopy)[i]);
		}
	}
	
	SetColumnInitializedFlag(true, UninitializedRow);

	return UninitializedRow;
}

inline int32 FArchetype::AddUninitialized(const int32 Num)
{
	check(Num > 0);

	const int32 OldNumColumns = NumColumns;
	NumColumns += Num;
	const SIZE_T NumBytes = FMath::DivideAndRoundUp<SIZE_T>(NumColumns, BITELEM_SIZE_BITS) * BITELEM_SIZE_BYTES;
	if (UNLIKELY(!InitializedColumnBitMask))
	{
		InitializedColumnBitMask = (FBitElem*)FMemory::MallocZeroed(NumBytes, alignof(FBitElem));
	}
	else
	{
		// Allocate new bytes for BitMask if necessary
		if (NumBytes != FMath::DivideAndRoundUp<SIZE_T>(OldNumColumns, BITELEM_SIZE_BITS) * BITELEM_SIZE_BYTES)
		{
			InitializedColumnBitMask = (FBitElem*)FMemory::Realloc(InitializedColumnBitMask, NumBytes, alignof(FBitElem));
		}

		// Set new bits to false
		for (int32 i = OldNumColumns; i < NumColumns; ++i)
		{
			SetColumnInitializedFlag(false, i);
		}
	}

	// Allocate new row elements
	ForEachRow([this](FComponentsRow& Row)->void
	{
		if (UNLIKELY(!Row.Memory))
		{
			Row.Memory = (uint8*)FMemory::Malloc(NumColumns * Row.GetSize(), Row.GetAlignment());
		}
		else
		{
			Row.Memory = (uint8*)FMemory::Realloc(Row.Memory, NumColumns * Row.GetSize(), Row.GetAlignment());
		}
	});

	return OldNumColumns;
}

inline int32 FArchetype::AddDefaulted(const int32 Num)
{
	const int32 FirstIndex = AddUninitialized(Num);

	// Set column bit flags to true for initialized
	for (int32 i = FirstIndex; i < NumColumns; ++i)
	{
		check(!IsColumnInitialized(i));
		SetColumnInitializedFlag(true, i);
	}

	// Initialize row elements
	ForEachRow([&FirstIndex, &Num](FComponentsRow& Row)
	{
		Row.ScriptStruct->InitializeStruct(Row[FirstIndex], Num);
	});
	
	return FirstIndex;
}

inline bool FArchetype::DestructAt(const int32 ColumnIndex)
{
	check(IsValidColumn(ColumnIndex));
	if (!IsColumnInitialized(ColumnIndex)) return false;

	// Set validity flag to false and destroy all corresponding components
	SetColumnInitializedFlag(false, ColumnIndex);
	ForEachRow([&ColumnIndex](FComponentsRow& Row)->void
	{
		Row.ScriptStruct->DestroyStruct(Row[ColumnIndex]);
	});

	return true;
}

template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T*>::Type FArchetype::Get(const int32 RowIndex, const int32 ColumnIndex)
{
	check(IsValidRow(RowIndex));
	check(IsValidColumn(ColumnIndex));
	return IsColumnInitialized(ColumnIndex) ? (T*)Rows[RowIndex][ColumnIndex] : nullptr;
}

template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, const T*>::Type FArchetype::Get(const int32 RowIndex, const int32 ColumnIndex) const
{
	return const_cast<FArchetype*>(this)->Get<T>(RowIndex, ColumnIndex);
}

template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, T*>::Type FArchetype::Get(const int32 ColumnIndex)
{
	const int32 RowIndex = Algo::IndexOfBy(TConstArrayView<FComponentsRow>(Rows, NumRows), T::StaticStruct(), &FComponentsRow::ScriptStruct);
	check(RowIndex != INDEX_NONE);
	return Get<T>(RowIndex, ColumnIndex);
}

template<typename T>
UE_NODISCARD FORCEINLINE typename TEnableIf<TIsDerivedFrom<T, FECSCompBase>::Value, const T*>::Type FArchetype::Get(const int32 ColumnIndex) const
{
	return const_cast<FArchetype*>(this)->Get<T>(ColumnIndex);
}

FORCEINLINE FArchetype::FComponentsRow& FArchetype::operator[](const int32 Index)
{
	check(IsValidRow(Index));
	return Rows[Index];
}

FORCEINLINE const FArchetype::FComponentsRow& FArchetype::operator[](const int32 Index) const
{
	check(IsValidRow(Index));
	return Rows[Index];
}

inline int32 FArchetype::GetCompRow(const FCompTypeID CompTypeID) const
{
	checkf(IncludedCompTagBitMask[CompTypeID.ToInt() / BITELEM_SIZE_BITS] & 1ull << CompTypeID.ToInt() % BITELEM_SIZE_BITS, TEXT("No components row exists for this ID!"));
	
	int32 RowIndex = 0;
	for (int32 i = CompTypeID.ToInt() - 1; i >= 0; --i)
		RowIndex += (bool)(IncludedCompTagBitMask[i / BITELEM_SIZE_BITS] & 1ull << i % BITELEM_SIZE_BITS);

	checkf(IsValidRow(RowIndex), TEXT("Calculated invalid row %i"), RowIndex);
	return RowIndex;
}

inline int32 FArchetype::FindFirstUninitializedRow(const int32 StartColumn) const
{
	check(StartColumn >= 0);
	
	const int32 NumBitElems = FMath::DivideAndRoundUp<int32>(NumColumns, BITELEM_SIZE_BITS);
	for (int32 i = StartColumn; i < NumBitElems; ++i)
	{
		unsigned long Value;
#if PLATFORM_64BITS
		if (_BitScanForward64(&Value, InitializedColumnBitMask[i]))
#else
		if (_BitScanForward(&Value, InitializedColumnBitMask[i]))
#endif
		{
			return Value + 1 + i * BITELEM_SIZE_BITS;
		}
	}

	return INDEX_NONE;
}

inline bool FArchetype::HasSameSetIdentifierFlags(const FArchetype& Other, const int32 NumCompsAndTags) const
{
	const int32 End = FMath::DivideAndRoundUp<int32>(NumCompsAndTags, BITELEM_SIZE_BITS);
	for (int32 i = 0; i < End; ++i)
		if ((IncludedCompTagBitMask[i] & Other.IncludedCompTagBitMask[i]) != IncludedCompTagBitMask[i])
			return false;

	return true;
}


inline void FArchetype::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	ForEachRow([&](FComponentsRow& Row)
	{
		Collector.AddReferencedObject(Row.ScriptStruct);
		if (!ensure(Row.ScriptStruct)) return;
		
		const bool bHasNativeAdd = Row.ScriptStruct->StructFlags & STRUCT_AddStructReferencedObjects;
		ForEachInitializedColumn([&](const int32 Index)
		{
			uint8* Item = Row[Index];
			if (bHasNativeAdd)
			{
				Row.ScriptStruct->GetCppStructOps()->AddStructReferencedObjects()(Item, Collector);
			}

			for (TPropertyValueIterator<FObjectProperty> It(Row.ScriptStruct, Item); It; ++It)
			{
				UObject*& ReferencedObject = *(UObject**)It.Value();
				Collector.AddReferencedObject(ReferencedObject);
			}
		});
	});
}

FORCEINLINE FArchetype::FComponentsRow* FArchetype::begin() { return Rows; }
FORCEINLINE const FArchetype::FComponentsRow* FArchetype::begin() const { return Rows; }
FORCEINLINE FArchetype::FComponentsRow* FArchetype::end() { return Rows + NumRows; }
FORCEINLINE const FArchetype::FComponentsRow* FArchetype::end() const { return Rows + NumRows; }

template<typename TFunctor>
inline void FArchetype::ForEachRow(TFunctor&& Functor)
{
	for (int32 i = 0; i < NumRows; i++)
		Forward<TFunctor>(Functor)(Rows[i]);
}

template<typename TFunctor>
inline void FArchetype::ForEachRow(TFunctor&& Functor) const
{
	for (int32 i = 0; i < NumRows; i++)
		Forward<TFunctor>(Functor)(Rows[i]);
}

template<typename TFunctor>
inline void FArchetype::ForEachInitializedColumn(TFunctor&& Functor) const
{
	for (int32 i = 0; i < NumColumns; i++)
		if (IsColumnInitialized(i))
			Forward<TFunctor>(Functor)(i);
}

template<typename TFunctor>
void FArchetype::ForEachInitializedColumnWithBreak(const int32 StartIndex, TFunctor&& Functor) const
{
	for (int32 i = StartIndex; i < NumColumns; i++)
		if (IsColumnInitialized(i))
			if (!Forward<TFunctor>(Functor)(i))
				break;
}










