#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandTrackingSubsystem.h"
#include "HandTrackingTestActor.generated.h"

UCLASS()
class SANDWICHTRAINER_API AHandTrackingTestActor : public AActor
{
	GENERATED_BODY()

public:
	AHandTrackingTestActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	// Subsystem 참조
	UPROPERTY()
	UHandTrackingSubsystem* HandSubsystem;

	// 마지막 데이터 저장
	FHandData LastHandData;

	// 콜백
	void OnHandDataReceived(const FHandData& Data);
};