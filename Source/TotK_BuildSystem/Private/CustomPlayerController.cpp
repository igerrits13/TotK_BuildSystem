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
	if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent() != nullptr && PhysicsHandle->GetGrabbedComponent()->GetOwner()->IsA(AMoveableObject::StaticClass())) {
		HeldObject = Cast<AMoveableObject>(PhysicsHandle->GetGrabbedComponent()->GetOwner());
		TrackMouseShake();
	}

	// Otherwise, make sure held object is null
	else if (HeldObject) {
		HeldObject = nullptr;
	}
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

	// If the mouse direction has changed enough, detect shake and split apart fused objects through the interface function
	if (DirectionChanges >= MaxDirectionChanges) {
		MouseDeltasX.Empty();
		MouseDeltasY.Empty();
		IMoveableObjectInterface::Execute_SplitMoveableObjects(HeldObject);
	}
}