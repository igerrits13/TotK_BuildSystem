// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomPlayerController.h"

#include "../DebgugHelper.h"

// Setup using the custom player controller
ACustomPlayerController::ACustomPlayerController()
{

}

// Called when the game starts or when spawned
void ACustomPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACustomPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TrackMouseShake();
}

// Detect mouse shake for breaking apart fused objects
void ACustomPlayerController::TrackMouseShake()
{
	// Get the x and y mouse deltas
	float MouseX, MouseY;
	GetInputMouseDelta(MouseX, MouseY);

	// Add x and y deltas to buffers, removing oldest data if buffer is full
	MouseDeltasX.Add(MouseX);
	if (MouseDeltasX.Num() > MaxSamples)
		MouseDeltasX.RemoveAt(0);

	MouseDeltasY.Add(MouseY);
	if (MouseDeltasY.Num() > MaxSamples)
		MouseDeltasY.RemoveAt(0);

	int DirectionChanges = 0;

	// Count the number of direction changes based on how far the mouse has moved and if it has changed direction
	for (int i = 1; i < MouseDeltasX.Num(); ++i) {
		if (FMath::Abs(MouseDeltasX[i]) > ShakeThreshold &&
			FMath::Sign(MouseDeltasX[i]) != FMath::Sign(MouseDeltasX[i - 1])) {
			DirectionChanges++;
		}
	}

	for (int i = 1; i < MouseDeltasY.Num(); ++i) {
		if (FMath::Abs(MouseDeltasY[i]) > ShakeThreshold &&
			FMath::Sign(MouseDeltasY[i]) != FMath::Sign(MouseDeltasY[i - 1])) {
			DirectionChanges++;
		}
	}

	// If the mouse direction has changed enough, detect shake
	if (DirectionChanges >= MaxDirectionChanges) {
		Debug::Print(TEXT("Shake detected!"));
		MouseDeltasX.Empty();
		MouseDeltasY.Empty();
	}
}