// Fill out your copyright notice in the Description page of Project Settings.


#include "SandwichPlayer.h"

// Sets default values
ASandwichPlayer::ASandwichPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASandwichPlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASandwichPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASandwichPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

