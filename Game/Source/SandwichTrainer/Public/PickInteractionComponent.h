// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PickInteractionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SANDWICHTRAINER_API UPickInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPickInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	// 카메라로부터 물체를 유지할 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pick)
	float HoldDistance = 80.0f;

	// 집기/놓기 raytrace 최대 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pick)
	float TraceDistance = 5000.0f;

	// 집고 있는 동안 적용할 스케일 배율 (1.0 = 원본 크기)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pick)
	float HoldScaleMultiplier = 0.3f;

private:
	void OnClickStarted();
	void OnClickCompleted();

	void TryPickActor();
	void Drop();

	bool GetWorldRay(FVector& OutLocation, FVector& OutDirection) const;

	TWeakObjectPtr<AActor> HeldActor;

	// 집기 전 상태 저장
	FVector OriginalScale = FVector::OneVector;
	bool bOriginalSimulatesPhysics = false;

	// 드롭 위치
	FVector LastValidDropLocation = FVector::ZeroVector;
	bool bHasValidDropLocation = false;

	UPROPERTY()
	TObjectPtr<class UPointerInputComponent> PointerInput;
};
