// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Pickable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPickable : public UInterface
{
	GENERATED_BODY()
};

class SANDWICHTRAINER_API IPickable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnPickedUp();
	virtual void OnPickedUp_Implementation() {}

	UFUNCTION(BlueprintNativeEvent)
	void OnDropped(const FVector& DropLocation);
	virtual void OnDropped_Implementation(const FVector& DropLocation) {}
};
