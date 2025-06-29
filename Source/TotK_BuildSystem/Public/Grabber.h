// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Grabber.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TOTK_BUILDSYSTEM_API UGrabber : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGrabber();

	UFUNCTION(BlueprintCallable)
	void Grab();

	UFUNCTION(BlueprintCallable)
	void Release();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float LineTraceOffset = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MaxGrabDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float GrabRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MinHoldDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MaxHoldDistance = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float RotationDegrees = 45.f;

	UFUNCTION(BlueprintCallable)
	void RotateLeft();

	UFUNCTION(BlueprintCallable)
	void RotateRight();

	UFUNCTION(BlueprintCallable)
	void RotateUp();

	UFUNCTION(BlueprintCallable)
	void RotateDown();

	UFUNCTION(BlueprintCallable)
	void MoveTowards();

	UFUNCTION(BlueprintCallable)
	void MoveAway();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Check if an item is currently grabbed
	bool IsHoldingObject();

private:

	// Parameters to ignore the player when running a line trace
	FCollisionQueryParams Params;

	// Current distance that an object is being held at
	float CurrentHoldDistance;

	// Player facing rotation of the held object
	FRotator HeldRotation;

	// Player adjusted rotation
	FQuat AdjustedRotation;

	// Physics handle for moving objects
	UPhysicsHandleComponent* PhysicsHandle;

	// Vector for offsetting the height of held objects caused by third-person camera
	FVector CameraOffsetVector;

	// Check if there is a grabbable object and return if there is
	bool GetGrabbableInReach(FHitResult& OutHitResult, FRotator& OutOwnerRotation) const;

	UMaterialInterface* Mat;

	UMaterialInstanceDynamic* DynamicMat;
};
