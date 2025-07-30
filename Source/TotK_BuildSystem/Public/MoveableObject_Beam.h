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
	
};
