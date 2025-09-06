// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CustomGameMode.generated.h"

/**
 *
 */
UCLASS()
class TOTK_BUILDSYSTEM_API ACustomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Setup using the custom game mode
	ACustomGameMode();

private:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
