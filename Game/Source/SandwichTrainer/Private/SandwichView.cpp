// Fill out your copyright notice in the Description page of Project Settings.


#include "SandwichView.h"
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

	// ...
	// 1. 현재 게임의 플레이어 컨트롤러를 가져옵니다.
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

	// ...
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	// GetOwner를 APawn으로 형변환(Cast)해서 사용해야 합니다.
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (PC && OwnerPawn)
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			int32 ViewportWidth, ViewportHeight;
			PC->GetViewportSize(ViewportWidth, ViewportHeight);

			float LeftThreshold = ViewportWidth * 0.1f;
			float RightThreshold = ViewportWidth * 0.9f;

			// 이제 OwnerPawn을 통해 회전 값을 전달합니다.
			if (MouseX < LeftThreshold)
			{
				OwnerPawn->AddControllerYawInput(-1.0f * DeltaTime * 100.0f);
			}
			else if (MouseX > RightThreshold)
			{
				OwnerPawn->AddControllerYawInput(1.0f * DeltaTime * 100.0f);
			}
		}
	}
}

