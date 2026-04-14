// Fill out your copyright notice in the Description page of Project Settings.

#include "PickInteractionComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UserExtension.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interface/Pickable.h"
#include "HandTrackingSubsystem.h"

UPickInteractionComponent::UPickInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPickInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (auto GI = GetOwner()->GetGameInstance())
	{
		HandSubsystem = GI->GetSubsystem<UHandTrackingSubsystem>();
		HandSubsystem->OnHandDataReceived.AddUObject(this, &UPickInteractionComponent::OnHandDataReceived);
		LOGW(TEXT("HandTracking Subsystem Connected"));
	}
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

void UPickInteractionComponent::OnClickStarted()
{
	LOGW(TEXT("OnClickStarted"));
	TryPickActor();
}

void UPickInteractionComponent::OnClickCompleted()
{
	LOGW(TEXT("OnClickCompleted"));
	Drop();
}

// ─── World Ray ────────────────────────────────────────────────────────────────

bool UPickInteractionComponent::GetWorldRay(FVector& OutLocation, FVector& OutDirection) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return false;

	if (!bUseMouse)
	{
		int32 ViewX, ViewY;
		PC->GetViewportSize(ViewX, ViewY);
		return PC->DeprojectScreenPositionToWorld(LastHandData.X * ViewX, LastHandData.Y * ViewY, OutLocation,
		                                          OutDirection);
	}

	return PC->DeprojectMousePositionToWorld(OutLocation, OutDirection);
}

// ─── Pick ─────────────────────────────────────────────────────────────────────

void UPickInteractionComponent::TryPickActor()
{
	FVector WorldLocation, WorldDirection;
	if (!GetWorldRay(WorldLocation, WorldDirection)) return;

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

	// 지정된 배율로 스케일 축소
	HitActor->SetActorScale3D(OriginalScale * HoldScaleMultiplier);

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

	FVector WorldLocation, WorldDirection;
	if (!GetWorldRay(WorldLocation, WorldDirection)) return;

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

	// 스케일 먼저 복원 (바운드 계산에 필요)
	Actor->SetActorScale3D(OriginalScale);

	// 마지막 유효 위치에 배치 — 바운드 하단이 바닥면과 맞닿도록 Z 보정
	if (bHasValidDropLocation)
	{
		Actor->SetActorLocation(LastValidDropLocation, false, nullptr, ETeleportType::TeleportPhysics);

		const FBox Bounds = Actor->GetComponentsBoundingBox(true);
		const float BottomOffset = Actor->GetActorLocation().Z - Bounds.Min.Z;
		const FVector FinalLocation = LastValidDropLocation + FVector(0.f, 0.f, BottomOffset);
		Actor->SetActorLocation(FinalLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 충돌/물리 복원
	Actor->SetActorEnableCollision(true);
	if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		PrimComp->SetSimulatePhysics(bOriginalSimulatesPhysics);
	}

	IPickable::Execute_OnDropped(Actor, LastValidDropLocation);

	HeldActor = nullptr;
	bHasValidDropLocation = false;
}

void UPickInteractionComponent::OnHandDataReceived(const FHandData& Data)
{
	// Sign 전환 감지
	if (LastHandData.Sign == TEXT("none") && Data.Sign == TEXT("grab"))
	{
		OnClickStarted();
	}
	else if (LastHandData.Sign == TEXT("grab") && Data.Sign == TEXT("none"))
	{
		OnClickCompleted();
	}

	LastHandData = Data;

	LOGW(TEXT("Sign: %s | X: %f | Y: %f"), *Data.Sign, Data.X, Data.Y);
}
