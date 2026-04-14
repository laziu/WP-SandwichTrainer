// Fill out your copyright notice in the Description page of Project Settings.

#include "PointerInputComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UserExtension.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UPointerInputComponent::UPointerInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPointerInputComponent::BeginPlay()
{
	Super::BeginPlay();

	if (auto GI = GetOwner()->GetGameInstance())
	{
		HandSubsystem = GI->GetSubsystem<UHandTrackingSubsystem>();
		HandSubsystem->OnHandDataReceived.AddUObject(this, &UPointerInputComponent::OnHandDataReceived);
	}
}

// ─── Input Binding ────────────────────────────────────────────────────────────

void UPointerInputComponent::SetupInputBindings(UEnhancedInputComponent* EIC)
{
	if (!ClickAction)
	{
		LOGW(TEXT("No ClickAction assigned to %s"), *GetName());
		return;
	}

	EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &UPointerInputComponent::HandleClickStarted);
	EIC->BindAction(ClickAction, ETriggerEvent::Completed, this, &UPointerInputComponent::HandleClickCompleted);
}

// ─── Source Switch ────────────────────────────────────────────────────────────

void UPointerInputComponent::SetUseMouse(bool bNewUseMouse)
{
	if (bUseMouse == bNewUseMouse) return;

	if (bGrabbing)
	{
		bGrabbing = false;
		OnClickCompleted.Broadcast();
	}

	bUseMouse = bNewUseMouse;
}

// ─── Tick ─────────────────────────────────────────────────────────────────────

void UPointerInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseMouse)
	{
		if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			if (auto* PC = Cast<APlayerController>(OwnerPawn->GetController()))
			{
				float MouseX, MouseY;
				if (PC->GetMousePosition(MouseX, MouseY))
				{
					int32 ViewX, ViewY;
					PC->GetViewportSize(ViewX, ViewY);
					if (ViewX > 0 && ViewY > 0)
					{
						X = MouseX / ViewX;
						Y = MouseY / ViewY;
					}
				}
			}
		}
	}
}

// ─── Mouse Click ──────────────────────────────────────────────────────────────

void UPointerInputComponent::HandleClickStarted()
{
	if (!bUseMouse) return;

	bGrabbing = true;
	OnClickStarted.Broadcast();
}

void UPointerInputComponent::HandleClickCompleted()
{
	if (!bUseMouse) return;

	bGrabbing = false;
	OnClickCompleted.Broadcast();
}

// ─── MediaPipe ────────────────────────────────────────────────────────────────

void UPointerInputComponent::OnHandDataReceived(const FHandData& Data)
{
	if (!bUseMouse)
	{
		X = Data.X;
		Y = Data.Y;

		if (LastHandData.Sign == TEXT("none") && Data.Sign == TEXT("grab"))
		{
			bGrabbing = true;
			OnClickStarted.Broadcast();
		}
		else if (LastHandData.Sign == TEXT("grab") && Data.Sign == TEXT("none"))
		{
			bGrabbing = false;
			OnClickCompleted.Broadcast();
		}
	}

	LastHandData = Data;
}
