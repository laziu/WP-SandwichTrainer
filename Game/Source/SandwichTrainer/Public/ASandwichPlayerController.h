// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ASandwichPlayerController.generated.h"


/**
 */
UCLASS()
class SANDWICHTRAINER_API ASandwichPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASandwichPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<class UInputMappingContext> InputMappingDefault;
};
