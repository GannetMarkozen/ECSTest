
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ECSCharacter.generated.h"

UCLASS(Abstract)
class ECSTEST_API AECSCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AECSCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface
};