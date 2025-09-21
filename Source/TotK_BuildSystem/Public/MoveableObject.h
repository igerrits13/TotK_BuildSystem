// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoveableObjectInterface.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/BoxComponent.h"
#include "MoveableObject.generated.h"

// Physics constraint link for tracking which objects are fused together
USTRUCT()
struct FPhysicsConstraintLink
{
	GENERATED_BODY()

	UPROPERTY()
	UPhysicsConstraintComponent* Constraint;

	UPROPERTY()
	AMoveableObject* ComponentA;

	UPROPERTY()
	AMoveableObject* ComponentB;

	// Operator overload for comparing FPhysicsConstraintLinks
	FORCEINLINE bool operator==(const FPhysicsConstraintLink& Other) const
	{
		return Constraint == Other.Constraint &&
			ComponentA == Other.ComponentA &&
			ComponentB == Other.ComponentB;
	}
};

UCLASS()
class TOTK_BUILDSYSTEM_API AMoveableObject : public AActor, public IMoveableObjectInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMoveableObject();

	// Mesh component for the moveable object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UStaticMeshComponent* MeshComponent;

protected:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Material applied to the mesh component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UMaterialInterface* Mat;

	// Collision box for detecting objects within a certain rainge
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	UBoxComponent* FuseCollisionBox;

	// Speed that the fused objects will move towards each other
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float InterpSpeed = 6.f;

	// Tollerance for fusing objects together
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float FuseTolerance = 1.f;

	// Boolean for if debug information should be shown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebugMode = true;

	// Current nearby moveable object
	UPROPERTY()
	AMoveableObject* ClosestNearbyMoveableObject;

	// Keep track of all fused objects
	UPROPERTY()
	TSet<AMoveableObject*> FusedObjects;

	// Keep track of all physics constraints created on this object
	TArray<FPhysicsConstraintLink> PhysicsConstraintLinks;

	// Track if the held object is trying to fuse with another object
	bool bIsFusing;

	// Closest point for fusing the held object to another nearby object
	FVector HeldClosestFusionPoint;

	// Offset center of held object to closest fusion point
	FVector HeldLocalOffset;

	// Closest point for fusing the other nearby object to athe held object
	FVector OtherClosestFusionPoint;

	// Offset center of held object to closest fusion point
	FVector OtherLocalOffset;

private:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Update the current velocities of the moveable object
	void UpdateVelocities();

	// When an object is grabbed, add an overlay material
	virtual void OnGrab_Implementation() override;

	// When the object is released, remove overalay material
	virtual void OnRelease_Implementation() override;

	// Split the fused object sets of the currently held object through moveable object interface
	virtual void SplitMoveableObjects_Implementation() override;

	// Get the closest moveable object for the current actor
	AMoveableObject* GetClosestMoveableObjectByActor(AMoveableObject* Object, TArray<AActor*> HitResults);

	// Run a line trace to check for a clear path between the hit actor and currently held object
	AMoveableObject* CheckMoveableObjectTrace(AMoveableObject* HitActor, AMoveableObject* FusedObject);

	// Get the closest object to what is currently held
	AMoveableObject* GetClosestMoveable(AMoveableObject* Held, AMoveableObject* ObjectA, AMoveableObject* ObjectB);

	// Update the closest collision points on the held object and the nearby fusion object
	void UpdateCollisionPoints();

	// Move objects being fused together via interpolation over time
	void InterpFusedObjects(float DeltaTime);

	// Get the distance betwen two moveable objects
	float GetObjectDistance(AMoveableObject* MoveableA, AMoveableObject* MoveableB);

	// Remove velocities from objects when dropping
	void RemoveObjectVelocity();

	// Update the physics constraints of the two objects being fused
	void UpdateConstraints(AMoveableObject* MoveableObject);

	// Create a new physics constraint to be used with the physics constraint link
	UPhysicsConstraintComponent* AddPhysicsConstraint(AMoveableObject* MoveableObject);

	// Create a new constraint link and add it to both objects being fused
	void AddConstraintLink(UPhysicsConstraintComponent* PhysicsConstraint, AMoveableObject* MoveableObject);

	// Merge the fused object sets of the currently held object and the one it is fusing with
	void MergeMoveableObjects(AMoveableObject* MoveableObject);

	// Remove all physics constraints from the held object
	void RemovePhysicsLink();

	// Update fused object sets based on their physics links
	void UpdateFusedSet();

	// Remove velocities on hit objects if they are another moveable object
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult);

	// Get the closest moveable object within the collision range
	UFUNCTION(BlueprintCallable)
	AMoveableObject* GetClosestMoveableObjectInRadius();

	// Update material of nearby fuseable object and its currently fused object set
	UFUNCTION(BlueprintCallable)
	void UpdateMoveableObjectMaterial(AMoveableObject* MoveableObject, bool Fuseable);

	// Remove material of nearby fuseable object and its currently fused object set
	UFUNCTION(BlueprintCallable)
	void RemoveMoveableObjectMaterial(AMoveableObject* MoveableObject);

	// Fuse current object group with the nearest fuseable object
	UFUNCTION(BlueprintCallable)
	virtual void FuseMoveableObjects(AMoveableObject* MoveableObject);

	// A dynamic material to apply to the current object, allowing for manipulation of parameters
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMat;

	// A dynamic material to apply to the nearby moveable object
	UPROPERTY()
	UMaterialInstanceDynamic* MoveableDynamicMat;

	// Most recent nearby moveable object
	UPROPERTY()
	AMoveableObject* PrevMoveableObject;

	// Track if a moveable object is grabbed or not
	bool bIsGrabbed = false;

	// Vectors to store the current velocity of the moveable object
	FVector PreviousVelocity;
	FVector PreviousAngularVelocity;
};