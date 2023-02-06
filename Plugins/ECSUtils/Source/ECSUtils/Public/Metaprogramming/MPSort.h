
#pragma once

#include "TypeID.h"
#include "Conditional.h"

template<class T, template<T,T> class C>
struct zComp {
    template<T a, T b>
    using result=C<a,b>;
    using type=T;
};

namespace meta {
    template <typename T, T, typename> struct prepend;

    template <typename T, T Add, template <T...> class Z, T... Is>  
    struct prepend<T, Add, Z<Is...>> {  
        using type = Z<Add, Is...>;  
    };

    template <typename T, typename Pack1, typename Pack2> struct concat;

    template <typename T, template <T...> class Z, T... Ts, T... Us>
    struct concat<T, Z<Ts...>, Z<Us...>> {
        using type = Z<Ts..., Us...>;
    };
}

template <typename T, typename Pack, template <T> class UnaryPredicate> struct filter;  

template <typename T, template <T...> class Z, template <T> class UnaryPredicate, T I, T... Is>  
struct filter<T, Z<I, Is...>, UnaryPredicate> : std::conditional_t<UnaryPredicate<I>::value,
    meta::prepend<T, I, typename filter<T, Z<Is...>, UnaryPredicate>::type>,
    filter<T, Z<Is...>, UnaryPredicate>
> {};

template <typename T, template <T...> class Z, template <T> class UnaryPredicate>  
struct filter<T, Z<>, UnaryPredicate> {  
    using type = Z<>;  
};

template <typename T, T A, T B, template <T, T> class Comparator, template <T, T> class... Rest>
struct composed_binary_predicates : std::conditional_t<
    Comparator<A,B>::value,
    std::true_type,
    std::conditional_t<
        Comparator<B,A>::value,
        std::false_type,
        composed_binary_predicates<T, A, B, Rest...>
    >
> {};

template <typename T, T A, T B, template <T, T> class Comparator>
struct composed_binary_predicates<T, A, B, Comparator> : Comparator<A,B> {};

template <typename T, typename Sequence, template <T, T> class... Preds> struct composed_sort;

template <typename T, template <T...> class Z, T N, T... Is, template <T, T> class... Predicates>  
struct composed_sort<T, Z<N, Is...>, Predicates...> {  // Using the quick sort method.
    template <T I> struct less_than : std::integral_constant<bool, composed_binary_predicates<T,I,N, Predicates...>::value> {};
    template <T I> struct more_than : std::integral_constant<bool, !composed_binary_predicates<T,I,N, Predicates...>::value> {};  
    using subsequence_less_than_N = typename filter<T, Z<Is...>, less_than>::type;
    using subsequence_more_than_N = typename filter<T, Z<Is...>, more_than>::type; 
    using type = typename meta::concat<T, typename composed_sort<T, subsequence_less_than_N, Predicates...>::type,  
        typename meta::prepend<T, N, typename composed_sort<T, subsequence_more_than_N, Predicates...>::type>::type 
    >::type;
};

template <typename T, template <T...> class Z, template <T, T> class... Predicates>  
struct composed_sort<T, Z<>, Predicates...> {  
    using type = Z<>;
};

// Testing
template <int...> struct sequence;

template <int I, int J>  // The standard less than.
struct less_than : std::conditional_t<(I < J), std::true_type, std::false_type> {};

template <int N>
struct N_first {
    template <int I, int J>
    struct result : std::conditional_t<(I == N && J != N), std::true_type, std::false_type> {};
};

template <int I, int J>  // 25 placed first before anything else.
using twentyfive_first = typename N_first<25>::template result<I,J>;

template <int N>
struct N_last {
    template <int I, int J>
    struct result : std::conditional_t<(I != N && J == N), std::true_type, std::false_type> {};
};

template <int I, int J>  // 16 to be placed after everything else.
using sixteen_last = typename N_last<16>::template result<I,J>;

template <int I, int J>  // Even numbers placed after all 25's, if any, have been placed (and the even numbers sorted in increasing value among themselves).
struct even_numbers : std::conditional_t<((I%2 == 0 && J%2 != 0) || (I%2 == 0 && J%2 == 0 && I < J)), std::true_type, std::false_type> {}; 


namespace MP
{
	namespace Internal
	{
		template<typename TTupleType, typename TSequenceType>
	    struct TSequenceToTuple;

	    template<typename TTupleType, typename T, T... TValues>
	    struct TSequenceToTuple<TTupleType, sequence<TValues...>>
	    {
	        using Type = TTuple<typename TTupleElement<TValues, TTupleType>::Type...>;
	    };
	}
	
	template<typename... Ts>
	struct TSortByIDs { using Type = typename composed_sort<int32, sequence<TTypeID<Ts>::Value...>, less_than>::type; };
}