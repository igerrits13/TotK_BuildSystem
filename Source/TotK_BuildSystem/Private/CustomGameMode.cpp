// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomGameMode.h"
#include "CustomPlayerController.h"

#include "../DebgugHelper.h"

// Setup using the custom game mode
ACustomGameMode::ACustomGameMode()
{
	PlayerControllerClass = ACustomPlayerController::StaticClass();
}

// Called when the game starts or when spawned
void ACustomGameMode::BeginPlay()
{
	Super::BeginPlay();
}