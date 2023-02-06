
#pragma once
#include "Types/ECSBaseTypes.h"

template<typename... Ts>
class TTypeList {};

template<typename... Ts>
class TCompTypes : public TTypeList<Ts...> { static_assert(TAnd<TIsDerivedFrom<Ts, FECSCompBase>...>::Value, "TCompTypes: All template arguments must be derived from FECSCompBase!"); };

template<typename... Ts>
class TReads : public TTypeList<Ts...> { static_assert(TAnd<TIsDerivedFrom<Ts, FECSCompBase>...>::Value, "TReads: All template arguments must be derived from FECSCompBase!"); };

template<typename... Ts>
class TWrites : public TTypeList<Ts...> { static_assert(TAnd<TIsDerivedFrom<Ts, FECSCompBase>...>::Value, "TWrites: All template arguments must be derived from FECSCompBase!"); };

template<typename... Ts>
class TTagTypes : public TTypeList<Ts...> { static_assert(TAnd<TIsDerivedFrom<Ts, FECSTagBase>...>::Value, "TTagTypes: All template arguments must be derived from FECSTagBase!"); };

template<typename T> struct TIsTCompTypes { static constexpr bool Value = false; };
template<typename... Ts> struct TIsTCompTypes<TCompTypes<Ts...>> { static constexpr bool Value = true; };

template<typename T> struct TIsTTagTypes { static constexpr bool Value = false; };
template<typename... Ts> struct TIsTTagTypes<TTagTypes<Ts...>> { static constexpr bool Value = true; };

template<typename... Ts>
FORCEINLINE constexpr SIZE_T GetTypeListNum(TTypeList<Ts...>&&) { return sizeof...(Ts); }