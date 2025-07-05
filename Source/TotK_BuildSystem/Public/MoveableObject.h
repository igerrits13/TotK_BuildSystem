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
	float TraceRadius = 350.f;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Create and apply new material instance when obejct is grabbed
	virtual void OnGrab_Implementation() override;

	// When the object is released, set its material back to the original
	virtual void OnRelease_Implementation() override;

private:
	// Check for any overlapping moveable (fuseable) objects
	UFUNCTION(BlueprintCallable)
	AMoveableObject* GetMoveableInRadius();

	// Check if there is a grabbable object in sphere trace
	UFUNCTION(BlueprintCallable)
	AMoveableObject* GetMoveableItem(TArray<FHitResult> HitResults, FVector TraceOrigin);

	// Update material of nearby fuseable object
	UFUNCTION(BlueprintCallable)
	void UpdateMoveableItemMaterial(AMoveableObject* MoveableMesh);

	// Remove material of nearby fuseable object
	UFUNCTION(BlueprintCallable)
	void RemoveMoveableItemMaterial(AMoveableObject* MoveableMesh);

	// Fuse with the nearest fuseable object
	UFUNCTION(BlueprintCallable)
	void FuseMoveableObjects(AMoveableObject* MoveableMesh);

	// A dynamic material to apply to the current object, allowing for manipulation of parameters
	UMaterialInstanceDynamic* DynamicMat;

	// A dynamic material to apply to the nearby moveable object
	UMaterialInstanceDynamic* MoveableDynamicMat;

	// Current nearby moveable object
	AMoveableObject* MoveableObject;

	// Most recent nearby moveable object
	AMoveableObject* FuseMesh;

	bool bIsGrabbed = false;
};
