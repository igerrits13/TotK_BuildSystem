// Fill out your copyright notice in the Description page of Project Settings.


#include "Grabber.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

#include "../DebgugHelper.h"

// Sets default values for this component's properties
UGrabber::UGrabber()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UGrabber::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the physics handle and allow rotation physics
	PhysicsHandle = GetOwner()->FindComponentByClass<UPhysicsHandleComponent>();
	PhysicsHandle->bRotationConstrained = false;
	PhysicsHandle->InterpolationSpeed = 5.f;

	// Vector for offsetting the height of held objects caused by third-person camera
	CameraOffsetVector = FVector(0.f, 0.f, GetOwner()->GetSimpleCollisionHalfHeight() * OffsetValue);

	// Store parameters to ignore the player when trying to grab
	Params.AddIgnoredActor(GetOwner());
}


// Called every frame
void UGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Do nothing if there is no valid physics handle or held object
	if (PhysicsHandle == nullptr || PhysicsHandle->GetGrabbedComponent() == nullptr) return;

	// Update the held object's location and rotation as well as the rotation of the player
	UpdateHeldObjectLocationAndRotation();
	UpdatePlayerRotation();
}

// Update the location and rotation of the held object
void UGrabber::UpdateHeldObjectLocationAndRotation()
{
	// Store the location of the player and where the held object should be located
	FVector PlayerLocation = GetOwner()->GetActorLocation();
	FVector TargetLocation = PlayerLocation + GetForwardVector() * CurrentHoldDistance + CameraOffsetVector;

	// Store the quaternion value for the rotation of the held object facing the player. This is backwards due to the way meshes were created in blender, as their forward vector is seemingly backwards
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(TargetLocation, PlayerLocation);
	FRotator AdjustedLookAtRotation = FRotator(0.f, LookAtRotation.Yaw, 0.f);
	FQuat AdjustedLookAtQuat = FQuat(AdjustedLookAtRotation);
	AdjustedLookAtQuat.Normalize();

	// Create the final rotation of the object by applying the offset to hold the original object rotation, then the manually adjusted 
	// rotation to show player rotations and then the adjust lookat rotation to keep the object facing the player as they turn.
	FQuat FinalQuat = AdjustedLookAtQuat * AdjustedQuat * OffsetQuat;
	FinalQuat.Normalize();

	// Set the location and rotation of the held object
	PhysicsHandle->SetTargetLocationAndRotation(TargetLocation, FinalQuat.Rotator());

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Draw debug lines for the held object
	if (bDebugMode) {
		// Draw arrow towards owner of the physics handle from the held object
		DrawDebugDirectionalArrow(GetWorld(), PhysicsHandle->GetGrabbedComponent()->GetComponentLocation(), PhysicsHandle->GetGrabbedComponent()->GetComponentLocation() + PhysicsHandle->GetGrabbedComponent()->GetForwardVector() * 100, 20, FColor::Green, false);
		FVector Start = PhysicsHandle->GetGrabbedComponent()->GetComponentLocation();
		FRotator DebugLookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, GetOwner()->GetActorLocation());
		FVector Direction = DebugLookAtRotation.Vector();
		FVector End = Start + (Direction * 100.0f);
		DrawDebugDirectionalArrow(GetWorld(), Start, End, 10, FColor::Purple, false, 0.f, 0, 2.f);

		// Draw lines up, right and forward for the held object
		DrawDebugLine(GetWorld(), TargetLocation, TargetLocation + FinalQuat.GetRightVector() * 100, FColor::Red);
		DrawDebugLine(GetWorld(), TargetLocation, TargetLocation + FinalQuat.GetUpVector() * 100, FColor::Green);
		DrawDebugLine(GetWorld(), TargetLocation, TargetLocation + FinalQuat.GetForwardVector() * 100, FColor::Blue);
	}
	////////////////////////////////////////////////////////////////////////////////////
}

// Update the rotation of the player to look at the currently held object
void UGrabber::UpdatePlayerRotation()
{
	FVector OwnerLocation;
	FRotator OwnerRotation;

	// Rotate the player towards where the camera is facing
	GetOwner()->GetActorEyesViewPoint(OwnerLocation, OwnerRotation);
	GetOwner()->SetActorRotation(FRotator(0, OwnerRotation.Yaw, 0));
}

// Grab item if there is one avaliable
void UGrabber::Grab()
{
	// Check to make sure there is a valid owner and physics handle
	if (!GetOwner() || PhysicsHandle == nullptr) return;

	// Initialize variable to store hit result and rotation in respect to where the object being picked up is at
	FHitResult HitResult;
	FRotator OwnerRotation;

	// If there is a valid hit and the object is a moveable object, grab the moveable object
	if (GetGrabbableInReach(HitResult, OwnerRotation) && HitResult.GetActor() && HitResult.GetActor()->IsA(AMoveableObject::StaticClass())) {
		AMoveableObject* MoveableObject = Cast<AMoveableObject>(HitResult.GetActor());

		// Rotate the player towrards the object being picked up
		GetOwner()->SetActorRotation(OwnerRotation);

		// Setup the initial location and rotation of the grabbed object
		if (!IsStandingOnObject(MoveableObject)) {
			Debug::Print("Not standing on object");
			GrabObject(MoveableObject);
		}

		else {
			Debug::Print("Standing on object");
		}
	}
}

// Check if there is a grabbable object and return if there is
bool UGrabber::GetGrabbableInReach(FHitResult& OutHitResult, FRotator& OutOwnerRotation) const
{
	// Initialize values for character's location and rotation
	FVector OwnerLocation;
	GetOwner()->GetActorEyesViewPoint(OwnerLocation, OutOwnerRotation);

	// Initialize vectors for line trace start and end
	FVector Start = OwnerLocation;
	FVector End = Start + GetForwardVector() * MaxGrabDistance + CameraOffsetVector;

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Draw line trace when trying to grab an object
	if (bDebugMode) {
		DrawDebugLine(
			GetWorld(),
			Start,
			End,
			FColor::Yellow,
			false,
			3.f
		);

		const int32 Steps = 10;
		for (int32 i = 0; i <= Steps; ++i)
		{
			FVector Point = FMath::Lerp(Start, End, i / float(Steps));
			DrawDebugSphere(GetWorld(), Point, GrabRadius, 8, FColor::Yellow, false, 2.0f);
		}
	}
	////////////////////////////////////////////////////////////////////////////////////

	// Check for collisions with moveable actors
	return GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeSphere(GrabRadius), Params);
}

// Check if the player is currently standing on the grabbed object
bool UGrabber::IsStandingOnObject(AMoveableObject* MoveableObject) const
{
	// Initialize vectors for line trace
	FVector Start = GetOwner()->GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 200.f);

	// Initialize variables for line trace
	FHitResult HitResult;
	FCollisionQueryParams Paramss;
	Paramss.AddIgnoredActor(GetOwner());

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Draw line trace when trying to grab an object
	if (bDebugMode) {
		DrawDebugLine(
			GetWorld(),
			Start,
			End,
			FColor::Red,
			false,
			3.f
		);

		const int32 Steps = 10;
		for (int32 i = 0; i <= Steps; ++i)
		{
			FVector Point = FMath::Lerp(Start, End, i / float(Steps));
			DrawDebugSphere(GetWorld(), Point, GrabRadius, 8, FColor::Yellow, false, 2.0f);
		}
	}
	////////////////////////////////////////////////////////////////////////////////////

	// Run line trace for where the player is standing
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Paramss);

	// If the hit result is a moveable object, check if the hit result is within the fused object set of any object below the player
	if (HitResult.GetActor() && HitResult.GetActor()->IsA(AMoveableObject::StaticClass())) {
		bool bStandingOnGrabbedObject = MoveableObject->FusedObjects.Contains(Cast<AMoveableObject>(HitResult.GetActor()));

		// Only return true if the player is not standing on the moveable object
		return bHit && bStandingOnGrabbedObject;
	}

	// If the hit result is not a moveable object, return false
	else {
		return false;
	}
}

// Grab the object, setting its initial location and rotation
void UGrabber::GrabObject(AMoveableObject* MoveableObject)
{
	// Get the component being grabbed and wake up rigid bodies
	UPrimitiveComponent* HitComponent = MoveableObject->MeshComponent;
	HitComponent->WakeAllRigidBodies();

	// Call OnGrab using the moveable objects interface
	IMoveableObjectInterface::Execute_OnGrab(MoveableObject);

	// Store the location of the player and held object
	FVector PlayerLocation = GetOwner()->GetActorLocation();
	FVector TargetLocation = HitComponent->GetComponentLocation();

	// Update the current hold distance to be the distance between the player and the held object with an offset of the closest point on the held object to the player
	FVector OutUnusedVec;
	float CenterDistance = FVector::Dist(PlayerLocation, TargetLocation);
	HoldOffset = CenterDistance - HitComponent->GetClosestPointOnCollision(PlayerLocation, OutUnusedVec);
	CurrentHoldDistance = CenterDistance + HoldOffset;

	// Store the lookat rotation from the player to the object. This is backwards due to the way meshes were created in blender, as their forward vector is seemingly backwards
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLocation + GetForwardVector() * CurrentHoldDistance + CameraOffsetVector, PlayerLocation);
	FRotator AdjustedLookAtRotation(0.f, LookAtRotation.Yaw, 0.f);
	FQuat AdjustedLookAtQuat = FQuat(AdjustedLookAtRotation);
	AdjustedLookAtQuat.Normalize();

	// Store the held objects initial rotation when picked up
	FRotator HeldRotation = HitComponent->GetComponentRotation();
	FQuat HeldQuat = FQuat(HeldRotation);
	HeldQuat.Normalize();

	// Store the offset between the look at rotation and the object's initial rotation, then round it based on set rotation degrees
	OffsetQuat = AdjustedLookAtQuat.Inverse() * HeldQuat;
	FRotator OffsetRotator = RoundObjectRotation(FRotator(OffsetQuat));
	OffsetQuat = FQuat(OffsetRotator);
	OffsetQuat.Normalize();

	// Reset the adjusted rotation when picking up a new object
	AdjustedQuat = FQuat::Identity;

	// Make sure the object is not held too closely
	if (CurrentHoldDistance < MinHoldDistance) {
		CurrentHoldDistance = MinHoldDistance;
	}

	// Grab the object with its current location and rotation
	PhysicsHandle->GrabComponentAtLocationWithRotation(
		HitComponent,
		NAME_None,
		TargetLocation,
		HitComponent->GetComponentRotation()
	);
}

// Round off the grabbed object's initial rotation based on the preferred rotation degrees
FRotator UGrabber::RoundObjectRotation(FRotator HeldRot)
{
	FRotator ResRotation;

	// Get all mods for calculations
	float PitchMod = FMath::Fmod(HeldRot.Pitch, RotationDegrees);
	float YawMod = FMath::Fmod(HeldRot.Yaw, RotationDegrees);
	float RollMod = FMath::Fmod(HeldRot.Roll, RotationDegrees);

	// Calculate the rounded rotations
	ResRotation.Pitch = CalculateRotation(HeldRot.Pitch, PitchMod);
	ResRotation.Yaw = CalculateRotation(HeldRot.Yaw, YawMod);
	ResRotation.Roll = CalculateRotation(HeldRot.Roll, RollMod);

	return ResRotation;
}

// Round off the specified pitch, yaw, or roll value
float UGrabber::CalculateRotation(float CurrRot, float CurrMod)
{
	float RetRot = 0.0f;
	float HalfRot = RotationDegrees / 2;

	// Calculate the rounded values based on the mod values for pitch, yaw, and roll. For example: With RotationDegrees set to 45:
	// If the currMod is close to 0, no adjustment is needed
	if (FMath::Abs(CurrMod) < 0.01f) {
		RetRot = 0;
	}

	// Calculate the rounded values based on the mod values for pitch, yaw, and roll. For example: With RotationDegrees set to 45:
	// If the current mod is less than -22.5, we want to calculate (held rotation value + (-RotationDegrees - current mod value)) -> (-80 + (-45 - (-80 mod 45 = -35)) = -90)
	else if (CurrMod < -HalfRot) {
		RetRot = CurrRot + (-RotationDegrees - CurrMod);
	}

	// If the current mod is between -22.5 and 0, we want to calculate (held rotation value - current mod value) -> (-147 - (-147 mod 45 = -12) = -135)
	// If the current mod is between 0 and 22.5, we want to calculate (held rotation value - current mod value) -> (94 - (94 mod 45 = 4) = 90)
	else if (CurrMod > -HalfRot && CurrMod < HalfRot) {
		RetRot = CurrRot - CurrMod;
	}

	// Otherwise, the current mod must be greater than 22.5, we want to calculate (held rotation value + (RotationDegrees - current mod value)) -> (68 + (45 - (68 mod 45 = 23)) = 90)
	else {
		RetRot = CurrRot + (RotationDegrees - CurrMod);
	}

	return RetRot;
}

// Release the currently grabbed item
void UGrabber::Release()
{
	// Check to make sure there is a valid physics handle with a grabbed object
	if (PhysicsHandle == nullptr || PhysicsHandle->GetGrabbedComponent() == nullptr) return;

	// Call OnRelease using the moveable objects interface
	IMoveableObjectInterface::Execute_OnRelease(PhysicsHandle->GetGrabbedComponent()->GetOwner());

	// Release the component
	PhysicsHandle->ReleaseComponent();
}

// Check if the player is currently holding an item
bool UGrabber::IsHoldingObject()
{
	return PhysicsHandle->GetGrabbedComponent() != nullptr;
}

// Rotate the currently held object to the left
void UGrabber::RotateLeft()
{
	FQuat DeltaRot = FQuat(FVector(0, 0, 1), FMath::DegreesToRadians(RotationDegrees));
	AdjustedQuat = DeltaRot * AdjustedQuat;
}

// Rotate the currently held object to the right
void UGrabber::RotateRight()
{
	FQuat DeltaRot = FQuat(FVector(0, 0, 1), FMath::DegreesToRadians(-RotationDegrees));
	AdjustedQuat = DeltaRot * AdjustedQuat;
}

// Rotate the currently held object up
void UGrabber::RotateUp()
{
	FQuat DeltaRot = FQuat(FVector(0, 1, 0), FMath::DegreesToRadians(-RotationDegrees));
	AdjustedQuat = DeltaRot * AdjustedQuat;
}

// Rotate the currently held object down
void UGrabber::RotateDown()
{
	FQuat DeltaRot = FQuat(FVector(0, 1, 0), FMath::DegreesToRadians(RotationDegrees));
	AdjustedQuat = DeltaRot * AdjustedQuat;
}

// Move the currently held object towards player
void UGrabber::MoveTowards()
{
	if (CurrentHoldDistance > MinHoldDistance + 50.f) {
		CurrentHoldDistance -= 50.f;
	}
	else {
		CurrentHoldDistance = MinHoldDistance;
	}
}

// Move the currently held object away from player
void UGrabber::MoveAway()
{
	if (CurrentHoldDistance < MaxHoldDistance - 50.f) {
		CurrentHoldDistance += 50.f;
	}
	else {
		CurrentHoldDistance = MaxHoldDistance;
	}
}