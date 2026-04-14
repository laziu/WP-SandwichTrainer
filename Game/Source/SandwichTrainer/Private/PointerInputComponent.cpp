// Fill out your copyright notice in the Description page of Project Settings.

#include "PointerInputComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UserExtension.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GameFramework/HUD.h"
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

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AHUD* HUD = PC->GetHUD())
		{
			HUD->OnHUDPostRender.AddUObject(this, &UPointerInputComponent::DrawPointerCircle);
		}
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

	LOGW(TEXT("X: %f | Y: %f | Pick: %d"), X, Y, bGrabbing);
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
	if (!bUseMouse && Data.X > 0.f && Data.Y > 0.f)
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

// ─── HUD Draw ─────────────────────────────────────────────────────────────────

void UPointerInputComponent::DrawPointerCircle(AHUD* HUD, UCanvas* Canvas)
{
	if (bUseMouse || !Canvas) return;

	const float CX = X * Canvas->SizeX;
	const float CY = Y * Canvas->SizeY;
	constexpr float Radius = 8.f;
	constexpr int32 Segments = 4;

	for (int32 i = 0; i < Segments; i++)
	{
		const float A0 = (2.f * PI * i) / Segments;
		const float A1 = (2.f * PI * (i + 1)) / Segments;

		FCanvasLineItem Line(
			FVector2D(CX + Radius * FMath::Cos(A0), CY + Radius * FMath::Sin(A0)),
			FVector2D(CX + Radius * FMath::Cos(A1), CY + Radius * FMath::Sin(A1)));
		Line.LineThickness = 2.f;
		Line.SetColor(FLinearColor::Green);
		Canvas->DrawItem(Line);
	}
}
