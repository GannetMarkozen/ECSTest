
#pragma once

#include "Utilities/Metaprogramming.h"
#include "ECSSubsystem.h"

template<typename InTReads, typename InTWrites = TWrites<>, typename InTTagTypes = TTagTypes<>>
class TCompQuery;

template<typename... InTReads, typename... InTWrites, typename... InTTagTypes>
class TCompQuery<TReads<InTReads...>, TWrites<InTWrites...>, TTagTypes<InTTagTypes...>>
{
	static_assert(sizeof...(InTReads) != 0 || sizeof...(InTWrites) != 0);

public:
	using FCompTypes = TCompTypes<InTReads..., InTWrites...>;
	using FTagTypes = TTagTypes<InTTagTypes...>;
	
	TCompQuery() = delete;
	explicit TCompQuery(const UECSSubsystem* Subsystem);

	template<typename FunctorType>
	void ForEach(FunctorType&& Functor) const;

private:
	template<typename T>
	T& InternalGetComp(const FArchetype& Archetype, const int32 ColumnIndex) const;
	
	UECSSubsystem const* const Subsystem;
};

template<typename... InTReads, typename... InTWrites, typename... InTTagTypes>
FORCEINLINE TCompQuery<TReads<InTReads...>, TWrites<InTWrites...>, TTagTypes<InTTagTypes...>>::TCompQuery(const UECSSubsystem* Subsystem)
	: Subsystem(Subsystem)
{
	check(Subsystem);
}

template<typename... InTReads, typename... InTWrites, typename... InTTagTypes> template<typename FunctorType>
inline void TCompQuery<TReads<InTReads...>, TWrites<InTWrites...>, TTagTypes<InTTagTypes...>>::ForEach(FunctorType&& Functor) const
{
	const FArchetypeID ID = Subsystem->GetArchetypeID<FCompTypes, FTagTypes>();
	const FArchetype& Archetype = Subsystem->GetArchetype(ID);
	
	for (const FArchetype& QueryArchetype : Subsystem->GetArchetypes())
	{
		if (!Archetype.HasSameSetIdentifierFlags(QueryArchetype, Subsystem->GetNumComps() + Subsystem->GetNumTags())) continue;
		QueryArchetype.ForEachInitializedColumn([this, &QueryArchetype, &Functor](const int32 ColumnIndex)
		{
			Forward<FunctorType>(Functor)(InternalGetComp<std::add_const_t<InTReads>>(QueryArchetype, ColumnIndex)..., InternalGetComp<InTWrites>(QueryArchetype, ColumnIndex)...);
		});
	}
}

template<typename... InTReads, typename... InTWrites, typename... InTTagTypes> template<typename T>
FORCEINLINE T& TCompQuery<TReads<InTReads...>, TWrites<InTWrites...>, TTagTypes<InTTagTypes...>>::InternalGetComp(const FArchetype& Archetype, const int32 ColumnIndex) const
{
	const int32 RowIndex = Archetype.GetCompRow(Subsystem->GetCompTypeID<std::remove_const_t<T>>());
	return *(T*)Archetype[RowIndex][ColumnIndex];
}



