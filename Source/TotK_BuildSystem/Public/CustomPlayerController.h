// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MoveableObject.h"
#include "CustomPlayerController.generated.h"

/**
 *
 */
UCLASS()
class TOTK_BUILDSYSTEM_API ACustomPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// Setup using the custom player controller
	ACustomPlayerController();

protected:
	// Max length of mouse delta buffer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grabbable")
	int MaxSamples = 50;

	// Max direction changes before shake is registered
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grabbable")
	int MaxDirectionChanges = 6;

	// Distance allowed between mouse movements before shake is detected
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grabbable")
	float ShakeThreshold = 0.1f;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

private:
	// Track the movement of the mouse to see if mouse shake has occured
	UFUNCTION(BlueprintCallable)
	void TrackMouseShake();

	// Reference to the object that is currently held by the player
	AMoveableObject* HeldObject;

	// Arrays for keeping track of the most recent x and y mouse movements
	TArray<float> MouseDeltasX;
	TArray<float> MouseDeltasY;
};
