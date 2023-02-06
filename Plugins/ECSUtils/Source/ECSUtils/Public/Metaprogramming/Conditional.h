
#pragma once

namespace MP
{
	template<bool Value, typename A, typename B>
	struct TConditional { using Type = A; };

	template<typename A, typename B>
	struct TConditional<false, A, B> { using Type = B; };

	template<bool Value, typename A, typename B>
	using TConditional_T = typename TConditional<Value, A, B>::Type;
}