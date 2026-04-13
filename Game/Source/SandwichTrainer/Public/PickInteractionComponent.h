// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/InputBindable.h"
#include "PickInteractionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SANDWICHTRAINER_API UPickInteractionComponent : public UActorComponent, public IInputBindable
{
	GENERATED_BODY()

public:
	UPickInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// IInputBindable
	virtual void SetupInputBindings(class UEnhancedInputComponent* EIC) override;

protected:
	virtual void BeginPlay() override;

	// 마우스 클릭 Input Action — BP에서 할당
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<class UInputAction> ClickAction;

	// 카메라로부터 물체를 유지할 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pick)
	float HoldDistance = 80.0f;

	// 집기/놓기 raytrace 최대 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pick)
	float TraceDistance = 5000.0f;

private:
	void OnClickStarted(const struct FInputActionValue& Value);
	void OnClickCompleted(const struct FInputActionValue& Value);

	void TryPickActor();
	void Drop();

	TWeakObjectPtr<AActor> HeldActor;

	// 집기 전 상태 저장
	FVector OriginalScale = FVector::OneVector;
	bool bOriginalSimulatesPhysics = false;

	// 드롭 위치
	FVector LastValidDropLocation = FVector::ZeroVector;
	bool bHasValidDropLocation = false;
};
