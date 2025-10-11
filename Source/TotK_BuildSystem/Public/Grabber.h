// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "MoveableObjectInterface.h"
#include "MoveableObject.h"
#include "Grabber.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TOTK_BUILDSYSTEM_API UGrabber : public USceneComponent, public IMoveableObjectInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGrabber();

	// Grab item if there is one avaliable
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void Grab();

	// Release the currently grabbed item
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void Release();

	// Rotate the currently held object to the left
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void RotateLeft();

	// Rotate the currently held object to the right
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void RotateRight();

	// Rotate the currently held object up
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void RotateUp();

	// Rotate the currently held object down
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void RotateDown();

	// Move the currently held object towards player
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void MoveTowards();

	// Move the currently held object away from player
	UFUNCTION(BlueprintCallable, Category = "Grab Settings")
	void MoveAway();

	// Check if the player is currently holding an item
	bool IsHoldingObject();

protected:
	// Boolean for if debug information should be shown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebugMode = true;

	// Offset for held objects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float OffsetValue = 1.5f;

	// Maximum distance that the grabber can grab an object from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MaxGrabDistance = 600.f;

	// Radius for checking overlapping objects during a line sweep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float GrabRadius = 10.f;

	// Minimum distance an object can be held from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MinHoldDistance = 400.f;

	// Maximum distance an object can be held from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MaxHoldDistance = 1200.f;

	// Degrees for each iteration of rotating the held object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float RotationDegrees = 45.f;

private:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Update the location and rotation of the held object
	void UpdateHeldObjectLocationAndRotation();

	// Update the rotation of the player to look at the currently held object
	void UpdatePlayerRotation();

	// Check if there is a grabbable object and return if there is
	bool GetGrabbableInReach(FHitResult& OutHitResult, FRotator& OutOwnerRotation) const;

	// Grab the object, setting its initial location and rotation
	void GrabObject(AMoveableObject* MoveableObject);

	// Physics handle for moving objects
	UPROPERTY()
	UPhysicsHandleComponent* PhysicsHandle;

	// Vector for offsetting the height of held objects caused by third-person camera
	FVector CameraOffsetVector;

	// Parameters to ignore the player when running a line trace
	FCollisionQueryParams Params;

	// Current distance that an object is being held at
	float CurrentHoldDistance;

	// Offset for the distance between the center of an object and its side closest to the player
	float HoldOffset;

	// Player facing rotation of the held object
	FRotator HeldRotation;

	// Player adjusted rotation based off of rotating the held object
	FQuat AdjustedRotation;
};