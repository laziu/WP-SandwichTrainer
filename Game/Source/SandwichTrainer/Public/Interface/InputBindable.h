// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InputBindable.generated.h"

// --- Interface ---

UINTERFACE(MinimalAPI)
class UInputBindable : public UInterface
{
	GENERATED_BODY()
};

class SANDWICHTRAINER_API IInputBindable
{
	GENERATED_BODY()

public:
	virtual void SetupInputBindings(class UEnhancedInputComponent* EIC) = 0;
};

