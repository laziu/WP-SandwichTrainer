// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/InputBindable.h"
#include "HandTrackingSubsystem.h"
#include "PointerInputComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SANDWICHTRAINER_API UPointerInputComponent : public UActorComponent, public IInputBindable
{
	GENERATED_BODY()

public:
	UPointerInputComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// IInputBindable
	virtual void SetupInputBindings(class UEnhancedInputComponent* EIC) override;

	// 런타임 소스 전환 — bGrabbing 초기화 및 OnClickCompleted 발동
	UFUNCTION(BlueprintCallable, Category=Input)
	void SetUseMouse(bool bNewUseMouse);

	// 정규화 화면 좌표 (0~1)
	float X = 0.f;
	float Y = 0.f;

	bool bGrabbing = false;

	FSimpleMulticastDelegate OnClickStarted;
	FSimpleMulticastDelegate OnClickCompleted;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	bool bUseMouse = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<class UInputAction> ClickAction;

private:
	void HandleClickStarted();
	void HandleClickCompleted();
	void OnHandDataReceived(const FHandData& Data);

	UPROPERTY()
	TObjectPtr<UHandTrackingSubsystem> HandSubsystem;

	FHandData LastHandData;
};
