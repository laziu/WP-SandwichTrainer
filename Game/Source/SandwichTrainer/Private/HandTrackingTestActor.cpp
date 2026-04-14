#include "HandTrackingTestActor.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

AHandTrackingTestActor::AHandTrackingTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHandTrackingTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		HandSubsystem = GI->GetSubsystem<UHandTrackingSubsystem>();

		if (HandSubsystem)
		{
			HandSubsystem->OnHandDataReceived.AddUObject(this, &AHandTrackingTestActor::OnHandDataReceived);
			UE_LOG(LogTemp, Log, TEXT("HandTracking Subsystem Connected"));
		}
	}
}

void AHandTrackingTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HandSubsystem)
	{
		HandSubsystem->OnHandDataReceived.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AHandTrackingTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 👉 좌표 기반으로 Actor 이동 테스트
	if (!LastHandData.Sign.IsEmpty())
	{
		const int32 SizeX = GEngine->GameViewport->Viewport->GetSizeXY().X;
		const int32 SizeY = GEngine->GameViewport->Viewport->GetSizeXY().Y;

		float X = LastHandData.X * SizeX;
		float Y = LastHandData.Y * SizeY;

		// 화면 좌표 → 월드 좌표 변환 (간단 버전)
		FVector NewLocation = FVector(X * 0.1f, Y * 0.1f, 100.f);

		SetActorLocation(NewLocation);
	}
}

void AHandTrackingTestActor::OnHandDataReceived(const FHandData& Data)
{
	LastHandData = Data;

	// 👉 로그 출력
	UE_LOG(LogTemp, Log, TEXT("Sign: %s | X: %f | Y: %f"),
		*Data.Sign, Data.X, Data.Y);

	// 👉 화면 출력 (디버그)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.1f,
			FColor::Green,
			FString::Printf(TEXT("Sign: %s (%.2f, %.2f)"),
				*Data.Sign, Data.X, Data.Y)
		);
	}
}