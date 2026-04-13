// Fill out your copyright notice in the Description page of Project Settings.

#include "ASandwichPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Interface/InputBindable.h"
#include "UserExtension.h"

ASandwichPlayerController::ASandwichPlayerController()
{
}

void ASandwichPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	InPawn->EnableInput(this);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InPawn->InputComponent))
	{
		for (UActorComponent* Comp : InPawn->GetComponents())
		{
			if (IInputBindable* Bindable = Cast<IInputBindable>(Comp))
			{
				Bindable->SetupInputBindings(EIC);
			}
		}
	}
}

void ASandwichPlayerController::OnUnPossess()
{
	if (APawn* MyPawn = GetPawn())
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(MyPawn->InputComponent))
		{
			EIC->ClearActionBindings();
		}
	}
	Super::OnUnPossess();
}

void ASandwichPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingDefault)
		{
			Subsystem->AddMappingContext(InputMappingDefault, 0);
		}
	}
}
