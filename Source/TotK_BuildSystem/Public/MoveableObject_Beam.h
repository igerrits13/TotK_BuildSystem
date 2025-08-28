// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MoveableObject.h"
#include "MoveableObject_Beam.generated.h"

/**
 *
 */
UCLASS()
class TOTK_BUILDSYSTEM_API AMoveableObject_Beam : public AMoveableObject
{
	GENERATED_BODY()

private:

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Fuse current object group with the nearest fuseable object
	virtual void FuseMoveableObjects(AMoveableObject* MoveableObject) override;

	// Update the physics constraints of the two objects being fused
	void UpdateConstraints(AMoveableObject* MoveableObject);

	// Update the closest collision points on the held object and the nearby fusion object
	void UpdateCollisionPoints();

	// Move objects being fused together via interpolation over time
	void InterpFusedObjects(float DeltaTime);
};
