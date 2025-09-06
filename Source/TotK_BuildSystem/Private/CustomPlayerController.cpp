// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomPlayerController.h"
#include "Grabber.h"

#include "../DebgugHelper.h"

// Setup using the custom player controller
ACustomPlayerController::ACustomPlayerController()
{

}

// Called when the game starts or when spawned
void ACustomPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the player character and physics handle
	PlayerCharacter = Cast<ATotK_BuildSystemCharacter>(GetPawn());
	if (PlayerCharacter) {
		PhysicsHandle = PlayerCharacter->FindComponentByClass<UPhysicsHandleComponent>();
	}
}

// Called every frame
void ACustomPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// If the player character is holding a moveable object, check for mouse shake
	if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent() != nullptr) {
		//If held object is null, get the currently grabbed component
		if (HeldObject == nullptr) {
			HeldObject = Cast<AMoveableObject>(PhysicsHandle->GetGrabbedComponent()->GetOwner());
		}
		TrackMouseShake();
	}

	// Otherwise, make sure held object is null
	else if (HeldObject != nullptr) {
		HeldObject = nullptr;
	}
}

// Detect mouse shake for breaking apart fused objects
void ACustomPlayerController::TrackMouseShake()
{
	// Get the x and y mouse deltas
	float MouseX, MouseY;
	GetInputMouseDelta(MouseX, MouseY);

	// Update the X and Y mouse movement buffers
	UpdateBuffers(MouseX, MouseY);

	// Count the total number of direction changes in the buffer to detect if mouse shake occured
	int DirectionChanges = CountMovementChanges();

	// If mouse shake was detected, empty the buffers and split the moveable objects using the MoveableObjectInterface
	if (DirectionChanges >= MaxDirectionChanges) {
		MouseDeltasX.Empty();
		MouseDeltasY.Empty();
		IMoveableObjectInterface::Execute_SplitMoveableObjects(HeldObject);
	}
}

// Add X and Y deltas to buffers, removing oldest data if buffer is full
void ACustomPlayerController::UpdateBuffers(float MouseX, float MouseY)
{
	MouseDeltasX.Add(MouseX);
	if (MouseDeltasX.Num() > MaxSamples)
		MouseDeltasX.RemoveAt(0);

	MouseDeltasY.Add(MouseY);
	if (MouseDeltasY.Num() > MaxSamples)
		MouseDeltasY.RemoveAt(0);
}

// Count the total number of changes in mouse direction movement
int ACustomPlayerController::CountMovementChanges()
{
	int DirectionChanges = 0;

	// Count the number of direction changes based on how far the mouse has moved on the X and Y and if the direction has changed
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

	return DirectionChanges;
}
