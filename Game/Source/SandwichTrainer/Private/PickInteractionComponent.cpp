// Fill out your copyright notice in the Description page of Project Settings.

#include "PickInteractionComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UserExtension.h"
#include "Interface/Pickable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UPickInteractionComponent::UPickInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPickInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

// ─── Input ───────────────────────────────────────────────────────────────────

void UPickInteractionComponent::SetupInputBindings(UEnhancedInputComponent* EIC)
{
	if (!ClickAction)
	{
		LOGW(TEXT("No ClickAction assigned to %s"), *GetName());
		return;
	}

	EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &UPickInteractionComponent::OnClickStarted);
	EIC->BindAction(ClickAction, ETriggerEvent::Completed, this, &UPickInteractionComponent::OnClickCompleted);
}

void UPickInteractionComponent::OnClickStarted(const FInputActionValue& Value)
{
	LOGW(TEXT("OnClickStarted"));
	TryPickActor();
}

void UPickInteractionComponent::OnClickCompleted(const FInputActionValue& Value)
{
	LOGW(TEXT("OnClickCompleted"));
	Drop();
}

// ─── Pick ─────────────────────────────────────────────────────────────────────

void UPickInteractionComponent::TryPickActor()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection)) return;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	const FVector TraceEnd = WorldLocation + WorldDirection * TraceDistance;
	if (!GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, Params))
		return;

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor || !HitActor->Implements<UPickable>())
		return;

	// 집기 전 상태 저장
	OriginalScale = HitActor->GetActorScale3D();
	if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(HitActor->GetRootComponent()))
	{
		bOriginalSimulatesPhysics = PrimComp->IsSimulatingPhysics();
		PrimComp->SetSimulatePhysics(false);
	}
	HitActor->SetActorEnableCollision(false);

	// 화면상 크기 유지를 위해 거리 비례 스케일 적용
	const float HitDistance = FVector::Dist(HitResult.ImpactPoint, WorldLocation);
	if (HitDistance > KINDA_SMALL_NUMBER)
	{
		const float ScaleFactor = HoldDistance / HitDistance;
		HitActor->SetActorScale3D(OriginalScale * ScaleFactor);
	}

	// 드롭 위치 초기값 = 집은 지점
	LastValidDropLocation = HitResult.ImpactPoint;
	bHasValidDropLocation = true;

	HeldActor = HitActor;
	IPickable::Execute_OnPickedUp(HitActor);
}

// ─── Tick ─────────────────────────────────────────────────────────────────────

void UPickInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HeldActor.IsValid()) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection)) return;

	// 1. 마우스를 따라 고정 거리에서 물체 이동
	const FVector NewLocation = WorldLocation + WorldDirection * HoldDistance;
	HeldActor->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// 2. 드롭 위치 갱신용 raytrace (HeldActor 무시)
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(HeldActor.Get());
	Params.AddIgnoredActor(GetOwner());

	const FVector TraceEnd = WorldLocation + WorldDirection * TraceDistance;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, Params))
	{
		LastValidDropLocation = HitResult.ImpactPoint;
		bHasValidDropLocation = true;
	}
}

// ─── Drop ─────────────────────────────────────────────────────────────────────

void UPickInteractionComponent::Drop()
{
	if (!HeldActor.IsValid()) return;

	AActor* Actor = HeldActor.Get();

	// 마지막 유효 위치에 배치
	if (bHasValidDropLocation)
	{
		Actor->SetActorLocation(LastValidDropLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 상태 복원
	Actor->SetActorScale3D(OriginalScale);
	Actor->SetActorEnableCollision(true);
	if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		PrimComp->SetSimulatePhysics(bOriginalSimulatesPhysics);
	}

	IPickable::Execute_OnDropped(Actor, LastValidDropLocation);

	HeldActor = nullptr;
	bHasValidDropLocation = false;
}
