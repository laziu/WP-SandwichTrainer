// Fill out your copyright notice in the Description page of Project Settings.

#include "PickInteractionComponent.h"

#include "PointerInputComponent.h"
#include "UserExtension.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interface/Pickable.h"

UPickInteractionComponent::UPickInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPickInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	PointerInput = GetOwner()->FindComponentByClass<UPointerInputComponent>();
	if (PointerInput)
	{
		PointerInput->OnClickStarted.AddUObject(this, &UPickInteractionComponent::OnClickStarted);
		PointerInput->OnClickCompleted.AddUObject(this, &UPickInteractionComponent::OnClickCompleted);
	}
	else
	{
		LOGW(TEXT("PointerInputComponent not found on %s"), *GetOwner()->GetName());
	}
}

// ─── Click ────────────────────────────────────────────────────────────────────

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
	if (!PointerInput) return false;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return false;

	int32 ViewX, ViewY;
	PC->GetViewportSize(ViewX, ViewY);
	return PC->DeprojectScreenPositionToWorld(
		PointerInput->X * ViewX,
		PointerInput->Y * ViewY,
		OutLocation, OutDirection);
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
	HeldPrimComp = nullptr;
	bOriginalSimulatesPhysics = false;

	TArray<UPrimitiveComponent*> PrimComps;
	HitActor->GetComponents<UPrimitiveComponent>(PrimComps);
	for (UPrimitiveComponent* Comp : PrimComps)
	{
		if (!Comp->IsSimulatingPhysics()) continue;

		// 물리 비활성화 및 속도 초기화
		Comp->SetSimulatePhysics(false);
		Comp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Comp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

		HeldPrimComp = Comp;
		bOriginalSimulatesPhysics = true;
		break;
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

	// 1. 커서 위치를 따라 고정 거리에서 물체 이동
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
		Actor->SetActorLocation(FinalLocation, true, nullptr, ETeleportType::None);
	}

	// 충돌/물리 복원
	Actor->SetActorEnableCollision(true);
	if (HeldPrimComp.IsValid() && bOriginalSimulatesPhysics)
		HeldPrimComp->SetSimulatePhysics(true);
	HeldPrimComp = nullptr;
	bOriginalSimulatesPhysics = false;

	IPickable::Execute_OnDropped(Actor, LastValidDropLocation);

	HeldActor = nullptr;
	bHasValidDropLocation = false;
}
