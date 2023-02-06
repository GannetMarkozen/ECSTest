
#pragma once

#include "CoreMinimal.h"
#include "ECSIDs.generated.h"

#define DEFINE_ECS_ID_TYPE(Name, SizeType) \
	static_assert(TIsSigned<SizeType>::Value); \
	FORCEINLINE constexpr Name() : ID(INDEX_NONE) {} \
	FORCEINLINE constexpr Name(const Name& Other) noexcept : ID(Other.ID) {} \
	FORCEINLINE constexpr Name(Name&& Other) noexcept : ID(Other.ID) {} \
	FORCEINLINE Name(ENoInit) {} \
	FORCEINLINE constexpr explicit Name(const SizeType ID) noexcept : ID(ID) {} \
	FORCEINLINE constexpr Name& operator=(const Name& Other) { ID = Other.ID; return *this; } \
	FORCEINLINE constexpr Name& operator=(Name&& Other) noexcept { ID = Other.ID; return *this; } \
	FORCEINLINE constexpr bool operator==(const Name& Other) const { return ID == Other.ID; } \
	FORCEINLINE constexpr bool operator!=(const Name& Other) const { return ID != Other.ID; } \
	FORCEINLINE constexpr bool operator>(const Name& Other) const { return ID > Other.ID; } \
	FORCEINLINE constexpr bool operator>=(const Name& Other) const { return ID >= Other.ID; } \
	FORCEINLINE constexpr bool operator<(const Name& Other) const { return ID < Other.ID; } \
	FORCEINLINE constexpr bool operator<=(const Name& Other) const { return ID <= Other.ID; } \
	FORCEINLINE constexpr explicit operator const SizeType&() const { return ID; } \
	FORCEINLINE constexpr const SizeType& ToInt() const noexcept { return ID; } \
	template<typename T, TEMPLATE_REQUIRES(TIsArithmetic<T>::Value)> \
	FORCEINLINE constexpr explicit operator T() const { return (T)ID; } \
	SizeType ID;

USTRUCT(BlueprintType)
struct ECSUTILS_API FEntityID
{
	GENERATED_BODY()
	DEFINE_ECS_ID_TYPE(FEntityID, int64);
};

USTRUCT(BlueprintType)
struct ECSUTILS_API FCompTypeID
{
	GENERATED_BODY()
	DEFINE_ECS_ID_TYPE(FCompTypeID, int32);
};

USTRUCT(BlueprintType)
struct ECSUTILS_API FTagTypeID
{
	GENERATED_BODY()
	DEFINE_ECS_ID_TYPE(FTagTypeID, int32);
};

USTRUCT(BlueprintType)
struct ECSUTILS_API FArchetypeID
{
	GENERATED_BODY()
	DEFINE_ECS_ID_TYPE(FArchetypeID, int32);
};