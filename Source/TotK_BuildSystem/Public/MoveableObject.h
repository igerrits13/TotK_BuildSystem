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

	// Material applied to the mesh component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UMaterialInterface* Mat;

	// Collision box for detecting objects within a certain rainge
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	UBoxComponent* FuseCollisionBox;

	// Boolean for if debug information should be shown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebugMode = true;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Keep track of all fused objects
	TSet<AMoveableObject*> FusedObjects;

	// Keep track of all physics constraints created on this object
	TArray<FPhysicsConstraintLink> PhysicsConstraintLinks;

	// Fuse current object group with the nearest fuseable object
	UFUNCTION(BlueprintCallable)
	virtual void FuseMoveableObjects(AMoveableObject* MoveableObject);

	// Merge the fused object sets of the currently held object and the one it is fusing with
	UFUNCTION(BlueprintCallable)
	void MergeMoveableObjects(AMoveableObject* MoveableObject);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// When an object is grabbed, add an overlay material
	virtual void OnGrab_Implementation() override;

	// When the object is released, remove overalay material
	virtual void OnRelease_Implementation() override;

	// Split the fused object sets of the currently held object through moveable object interface
	virtual void SplitMoveableObjects_Implementation() override;

	// Helper function to add constraint links
	void AddConstraintLink(const FPhysicsConstraintLink& Link);

private:
	// Remove velocities on hit objects if they are another moveable object
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult);

	//  Check for any nearby moveable objects for each object in the currently held object's fused group
	UFUNCTION(BlueprintCallable)
	AMoveableObject* GetMoveableInRadius();

	// Get closest moveable object
	UFUNCTION(BlueprintCallable)
	AMoveableObject* GetMoveableObject(TArray<AActor*> HitResults, FVector TraceOrigin);

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

	// Remove all physics constraints from the held object
	void RemovePhysicsLink();

	// Update fused object sets based on their physics links
	void UpdateFusedSet();

	// Vectors to store the current velocity of the moveable object
	FVector PreviousVelocity;
	FVector PreviousAngularVelocity;
};
