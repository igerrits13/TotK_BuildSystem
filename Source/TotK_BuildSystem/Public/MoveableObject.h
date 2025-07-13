// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoveableObjectInterface.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "MoveableObject.generated.h"

UCLASS()
class TOTK_BUILDSYSTEM_API AMoveableObject : public AActor, public IMoveableObjectInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMoveableObject();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UMaterialInterface* Mat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fuse")
	float TraceRadius = 150.f;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// When an object is grabbed, add an overlay material
	virtual void OnGrab_Implementation() override;

	// When the object is released, remove overalay material
	virtual void OnRelease_Implementation() override;

private:
	//  Check for any nearby moveable objects for each object in the currently held object's fused group
	UFUNCTION(BlueprintCallable)
	AMoveableObject* GetMoveableInRadius();

	// Get closest moveable object
	UFUNCTION(BlueprintCallable)
	AMoveableObject* GetMoveableObject(TArray<FHitResult> HitResults, FVector TraceOrigin);

	// Run a line trace to check if nearby moveable object is a valid hit
	UFUNCTION(BlueprintCallable)
	AMoveableObject* CheckMoveableObjectTrace(AActor* HitActor, FVector TraceOrigin);

	// Remove velocities from objects when dropping
	UFUNCTION(BlueprintCallable)
	void RemoveObjectVelocity();

	// Update material of nearby fuseable object and its currently fused object set
	UFUNCTION(BlueprintCallable)
	void UpdateMoveableObjectMaterial(AMoveableObject* MoveableObject, bool Fuseable);

	// Remove material of nearby fuseable object and its currently fused object set
	UFUNCTION(BlueprintCallable)
	void RemoveMoveableObjectMaterial(AMoveableObject* MoveableObject);

	// Fuse current object group with the nearest fuseable object
	UFUNCTION(BlueprintCallable)
	void FuseMoveableObjects(AMoveableObject* MoveableObject);

	// Merge the fused object sets of the currently held object and the one it is fusing with
	UFUNCTION(BlueprintCallable)
	void MergeMoveableObjects(AMoveableObject* MoveableObject);

	// A dynamic material to apply to the current object, allowing for manipulation of parameters
	UMaterialInstanceDynamic* DynamicMat;

	// A dynamic material to apply to the nearby moveable object
	UMaterialInstanceDynamic* MoveableDynamicMat;

	// Current nearby moveable object
	AMoveableObject* CurrentMoveableObject;

	// Most recent nearby moveable object
	AMoveableObject* PrevMoveableObject;

	// Track if a moveable object is grabbed or not
	bool bIsGrabbed = false;

	// Keep track of all fused objects
	TSet<AMoveableObject*> FusedObjects;
};
