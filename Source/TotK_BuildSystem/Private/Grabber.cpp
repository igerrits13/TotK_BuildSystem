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
	FVector TargetLocation = GetOwner()->GetActorLocation() + GetForwardVector() * CurrentHoldDistance + CameraOffsetVector;

	// Store the rotation that the object should have and combine with the overall rotation applied by the player
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(TargetLocation, PlayerLocation);
	FRotator AdjustedLookAtRotation = FRotator(0.f, LookAtRotation.Yaw, 0.f);
	FQuat FinalQuat = FQuat(AdjustedLookAtRotation) * AdjustedRotation;

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
		GrabObject(MoveableObject);
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
	FVector ObjectLocation = HitComponent->GetComponentLocation();

	// Update the current hold distance to the distance between the player and the held object with an offset of the closest point on the held object to the player
	FVector OutUnusedVec;
	float CenterDistance = FVector::Dist(PlayerLocation, ObjectLocation);
	HoldOffset = CenterDistance - HitComponent->GetClosestPointOnCollision(PlayerLocation, OutUnusedVec);
	CurrentHoldDistance = CenterDistance + HoldOffset;
	
	// Reset the adjusted rotation when picking up a new object
	AdjustedRotation = FQuat::Identity;

	// Make sure the object is not held too closely
	if (CurrentHoldDistance < MinHoldDistance) {
		CurrentHoldDistance = MinHoldDistance;
	}

	// Grab the object with its current location and rotation
	PhysicsHandle->GrabComponentAtLocationWithRotation(
		HitComponent,
		NAME_None,
		ObjectLocation,
		HitComponent->GetComponentRotation()
	);
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
	FQuat FacingQuat = FQuat(FRotator(0.f, HeldRotation.Yaw, 0.f));
	FVector UpVec = FacingQuat.GetUpVector();

	FQuat DeltaRot = FQuat(UpVec, FMath::DegreesToRadians(RotationDegrees));
	AdjustedRotation = DeltaRot * AdjustedRotation;
}

// Rotate the currently held object to the right
void UGrabber::RotateRight()
{
	FQuat FacingQuat = FQuat(FRotator(0.f, HeldRotation.Yaw, 0.f));
	FVector UpVec = FacingQuat.GetUpVector();

	FQuat DeltaRot = FQuat(UpVec, FMath::DegreesToRadians(-RotationDegrees));
	AdjustedRotation = DeltaRot * AdjustedRotation;
}

// Rotate the currently held object up
void UGrabber::RotateUp()
{
	FQuat FacingQuat = FQuat(FRotator(0.f, HeldRotation.Yaw, 0.f));
	FVector RightVec = FacingQuat.GetRightVector();

	FQuat DeltaRot = FQuat(RightVec, FMath::DegreesToRadians(-RotationDegrees));
	AdjustedRotation = DeltaRot * AdjustedRotation;

}

// Rotate the currently held object down
void UGrabber::RotateDown()
{
	FQuat FacingQuat = FQuat(FRotator(0.f, HeldRotation.Yaw, 0.f));
	FVector RightVec = FacingQuat.GetRightVector();

	FQuat DeltaRot = FQuat(RightVec, FMath::DegreesToRadians(RotationDegrees));
	AdjustedRotation = DeltaRot * AdjustedRotation;
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