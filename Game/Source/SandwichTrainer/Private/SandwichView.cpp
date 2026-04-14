// Fill out your copyright notice in the Description page of Project Settings.


#include "SandwichView.h"
#include "PointerInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
USandwichView::USandwichView()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USandwichView::BeginPlay()
{
	Super::BeginPlay();

	PointerInput = GetOwner()->FindComponentByClass<UPointerInputComponent>();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (PC)
	{
		// 2. 마우스 커서를 보이게 설정 (Show Mouse Cursor = true)
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;

		// 3. 입력 모드를 Game and UI로 설정
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);

		PC->SetInputMode(InputMode);
	}
}


// Called every frame
void USandwichView::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PointerInput) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	const float X = PointerInput->X;

	if (X < 0.1f)
	{
		OwnerPawn->AddMovementInput(-GetOwner()->GetActorRightVector(), 1.0f);
	}
	else if (X > 0.9f)
	{
		OwnerPawn->AddMovementInput(GetOwner()->GetActorRightVector(), 1.0f);
	}
}
