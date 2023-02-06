
#pragma once

#include "CoreMinimal.h"
#include "AnyStructArray.h"
#include "AnyStructArrayBPUtils.generated.h"

#define ANYSTRUCT_TEST_VALUE_COMPATIBILITY \
	if (UNLIKELY(!Value || !ValueProp)) \
	{ \
		Stack.bArrayContextFailed = true; \
		return; \
	} \
	if (UNLIKELY(!Any.GetType())) \
	{ \
	UE_LOG(LogBlueprint, Error, TEXT("Get (AnyStructArray): Attempted to access untyped AnyStructArray!")); \
	Stack.bArrayContextFailed = true; \
	return; \
	} \
	if (UNLIKELY(!Any.GetType()->IsChildOf(ValueProp->Struct))) \
	{ \
		UE_LOG(LogBlueprint, Error, TEXT("Get (AnyStructArray): Attempted to reinterpret cast AnyStructArray of type %s to %s!"), *Any.GetType()->GetName(), *ValueProp->Struct->GetName()); \
		Stack.bArrayContextFailed = true; \
		return; \
	}

#define ANYSTRUCT_TEST_ARRAY_COMPATIBILITY \
	if (UNLIKELY(!Arr || !ArrProp || !ArrProp->Inner->IsA(FStructProperty::StaticClass()) || !CastFieldChecked<FStructProperty>(ArrProp->Inner)->Struct)) \
	{ \
		Stack.bArrayContextFailed = true; \
		return; \
	} \
	if (UNLIKELY(!Any.GetType()->IsChildOf(CastFieldChecked<FStructProperty>(ArrProp->Inner)->Struct))) \
    { \
    	UE_LOG(LogBlueprint, Error, TEXT("Get (AnyStructArray): Attempted to reinterpret cast AnyStructArray of type %s to %s!"), *Any.GetType()->GetName(), *CastFieldChecked<FStructProperty>(ArrProp->Inner)->Struct->GetName()); \
    	Stack.bArrayContextFailed = true; \
    	return; \
    }

UCLASS(MinimalAPI)
class UAnyStructArrayUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, CustomThunk, DisplayName="Make AnyStructArray", Category="Utilities|AnyStructArray", meta=(ArrayParm="A"))
	static void MakeAnyStructArray(const TArray<int32>& A, FAnyStructArray& ReturnValue);
	DECLARE_FUNCTION(execMakeAnyStructArray)
	{
		Stack.StepCompiledIn<FArrayProperty>(nullptr);
		const uint8* Arr = Stack.MostRecentPropertyAddress;
		const FArrayProperty* ArrProp = CastField<FArrayProperty>(Stack.MostRecentProperty);

		Stack.StepCompiledIn<FStructProperty>(nullptr);
		FAnyStructArray& Any = *(FAnyStructArray*)Stack.MostRecentPropertyAddress;

		P_FINISH

		ANYSTRUCT_TEST_ARRAY_COMPATIBILITY

		P_NATIVE_BEGIN

		FScriptArrayHelper ArrayHelper(ArrProp, Arr);
		new (&Any) FAnyStructArray(CastFieldChecked<FStructProperty>(ArrProp->Inner)->Struct, ArrayHelper.GetRawPtr(), ArrayHelper.Num());

		P_NATIVE_END
	}

	UFUNCTION(BlueprintCallable, CustomThunk, DisplayName="Add (AnyStructArray)", Category="Utilities|AnyStructArray", meta=(CustomStructureParam="Value", CompactNodeTitle="ADD"))
	static UPARAM(DisplayName="Index") int32 AddAnyStructArray(UPARAM(ref) FAnyStructArray& Any, const int32& Value);
	DECLARE_FUNCTION(execAddAnyStructArray)
	{
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		FAnyStructArray& Any = *(FAnyStructArray*)Stack.MostRecentPropertyAddress;
		
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		const uint8* Value = Stack.MostRecentPropertyAddress;
		const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);

		P_FINISH

		ANYSTRUCT_TEST_VALUE_COMPATIBILITY

		P_NATIVE_BEGIN

		*(int32*)RESULT_PARAM = Any.AddFromBuffer(Value);

		P_NATIVE_END
	}

	UFUNCTION(BlueprintCallable, CustomThunk, DisplayName="Append (AnyStructArray)", Category="Utilities|AnyStructArray", meta=(ArrayParm="Array", CompactNodeTitle="APPEND"))
	static void AppendAnyStructArray(UPARAM(ref) FAnyStructArray& Any, const TArray<int32>& Array);
	DECLARE_FUNCTION(execAppendAnyStructArray)
	{
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		FAnyStructArray& Any = *(FAnyStructArray*)Stack.MostRecentProperty;
		
		Stack.StepCompiledIn<FArrayProperty>(nullptr);
		const uint8* Arr = Stack.MostRecentPropertyAddress;
		const FArrayProperty* ArrProp = CastField<FArrayProperty>(Stack.MostRecentProperty);

		P_FINISH

		if (UNLIKELY(!Arr || !ArrProp || !ArrProp->Inner->IsA(FStructProperty::StaticClass()) || !CastFieldChecked<FStructProperty>(ArrProp->Inner)->Struct))
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		if (UNLIKELY(!Any.GetType()->IsChildOf(CastFieldChecked<FStructProperty>(ArrProp->Inner)->Struct)))
		{
			UE_LOG(LogBlueprint, Error, TEXT("Get (AnyStructArray): Attempted to reinterpret cast AnyStructArray of type %s to %s!"), *Any.GetType()->GetName(), *CastFieldChecked<FStructProperty>(ArrProp->Inner)->Struct->GetName());
			Stack.bArrayContextFailed = true;
			return;
		}
		
		P_NATIVE_BEGIN

		FScriptArrayHelper ArrHelper(ArrProp, Arr);
		Any.AppendFromBuffer(ArrHelper.GetRawPtr(), ArrHelper.Num());

		P_NATIVE_END
	}

	UFUNCTION(BlueprintPure, CustomThunk, DisplayName="Get (AnyStructArray)", Category="Utilities|AnyStructArray", meta=(CustomStructureParam="Value", CompactNodeTitle="GET"))
	static void GetAnyStructArray(const FAnyStructArray& Any, const int32 Index, int32& Value);
	DECLARE_FUNCTION(execGetAnyStructArray)
	{
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		const FAnyStructArray& Any = *(FAnyStructArray*)Stack.MostRecentPropertyAddress;

		int32 Index;
		Stack.StepCompiledIn<FIntProperty>(&Index);

		Stack.StepCompiledIn<FStructProperty>(nullptr);
		uint8* Value = Stack.MostRecentPropertyAddress;
		const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);

		P_FINISH

		ANYSTRUCT_TEST_VALUE_COMPATIBILITY
		if (UNLIKELY(!Any.IsValidIndex(Index)))
		{
			UE_LOG(LogBlueprint, Error, TEXT("Get (AnyStructArray): Attempted to access invalid index %i of array length %i!"), Index, Any.Num());
			Stack.bArrayContextFailed = true;
			return;
		}

		P_NATIVE_BEGIN

		Any.GetType()->CopyScriptStruct(Value, Any.GetRawPtr(Index));

		P_NATIVE_END
	}

	UFUNCTION(BlueprintPure, CustomThunk, DisplayName="Num (AnyStructArray)", Category="Utilities|AnyStructArray", meta=(CompactNodeTitle="NUM"))
	static int32 NumAnyStructArray(const FAnyStructArray& Any);
	DECLARE_FUNCTION(execNumAnyStructArray)
	{
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		const FAnyStructArray& Any = *(FAnyStructArray*)Stack.MostRecentPropertyAddress;

		P_FINISH;
		P_NATIVE_BEGIN
		*(int32*)RESULT_PARAM = Any.Num();
		P_NATIVE_END
	}
};