// Fill out your copyright notice in the Description page of Project Settings.


#include "Grabber.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "MoveableObject.h"

#include "../DebgugHelper.h"

// Sets default values for this component's properties
UGrabber::UGrabber()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
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
	CameraOffsetVector = FVector(0.f, 0.f, GetOwner()->GetSimpleCollisionHalfHeight() * 1.5);

	// Store parameters to ignore the player when trying to grab
	Params.AddIgnoredActor(GetOwner());
}


// Called every frame
void UGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Do nothing if there is no valid physics handle or held item
	if (PhysicsHandle == nullptr || PhysicsHandle->GetGrabbedComponent() == nullptr) return;

	// Debugging lines for testing
	DrawDebugDirectionalArrow(GetWorld(), PhysicsHandle->GetGrabbedComponent()->GetComponentLocation(), PhysicsHandle->GetGrabbedComponent()->GetComponentLocation() + PhysicsHandle->GetGrabbedComponent()->GetForwardVector() * 100, 20, FColor::Green, false);
	FVector Start = PhysicsHandle->GetGrabbedComponent()->GetComponentLocation();
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, GetOwner()->GetActorLocation());
	FVector Direction = LookAtRotation.Vector();
	FVector End = Start + (Direction * 100.0f);
	DrawDebugDirectionalArrow(GetWorld(), Start, End, 10, FColor::Purple, false, 0.f, 0, 2.f);

	// Store the location of the player and where the held object should be held at
	FVector PlayerLocation = GetOwner()->GetActorLocation();
	FVector TargetLocation = GetOwner()->GetActorLocation() + GetForwardVector() * CurrentHoldDistance + CameraOffsetVector;

	// Store the rotation orientation that the object should have and combine with the overall rotation applied by the player
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(TargetLocation, PlayerLocation);
	FRotator AdjustedLookAtRot = FRotator(0.f, LookAtRot.Yaw, 0.f);
	FQuat FinalQuat = FQuat(AdjustedLookAtRot) * AdjustedRotation;

	// Set the location and rotation of the held object
	PhysicsHandle->SetTargetLocationAndRotation(TargetLocation, FinalQuat.Rotator());

	DrawDebugLine(GetWorld(), TargetLocation, TargetLocation + FinalQuat.GetRightVector() * 100, FColor::Red);
	DrawDebugLine(GetWorld(), TargetLocation, TargetLocation + FinalQuat.GetUpVector() * 100, FColor::Green);
	DrawDebugLine(GetWorld(), TargetLocation, TargetLocation + FinalQuat.GetForwardVector() * 100, FColor::Blue);

	// Initialize variable to store hit result from out parameter and rotation in respect to where the object being picked up is at
	FVector OwnerLocation;
	FRotator OwnerRotation;

	GetOwner()->GetActorEyesViewPoint(OwnerLocation, OwnerRotation);
	GetOwner()->SetActorRotation(FRotator(0, OwnerRotation.Yaw, 0));
}

// Grab item if able
void UGrabber::Grab()
{
	// Exit if there is no valid owner or physics handle
	if (!GetOwner() || PhysicsHandle == nullptr) return;

	// Initialize variable to store hit result and rotation in respect to where the object being picked up is at
	FHitResult HitResult;
	FRotator OwnerRotation;

	// If there is a valid hit and the object is a moveable object
	if (GetGrabbableInReach(HitResult, OwnerRotation) && HitResult.GetActor() && HitResult.GetActor()->IsA(AMoveableObject::StaticClass())) {
		AMoveableObject* MoveableObject = Cast<AMoveableObject>(HitResult.GetActor());

		// Rotate the player towrards the object being picked up
		GetOwner()->SetActorRotation(OwnerRotation);

		// Get the component being grabbed and wake up rigid bodies
		UPrimitiveComponent* HitComponent = MoveableObject->MeshComponent;
		HitComponent->WakeAllRigidBodies();

		// Call OnGrab using the moveable objects interface
		IMoveableObjectInterface::Execute_OnGrab(MoveableObject);

		// Find information for setting the held objects location and rotation and reset adjusted rotation
		FVector PlayerLocation = GetOwner()->GetActorLocation();
		FVector ObjectLocation = HitComponent->GetComponentLocation();
		CurrentHoldDistance = FVector::Dist(PlayerLocation, ObjectLocation);
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

	// For debugging
	else if (HitResult.GetActor() && !HitResult.GetActor()->IsA(AMoveableObject::StaticClass())) {
		Debug::Print(TEXT("Not a moveable object"));
	}
}

// Release grabbed item
void UGrabber::Release()
{
	// Exit if there is no valid physics handle or no held object
	if (PhysicsHandle == nullptr || PhysicsHandle->GetGrabbedComponent() == nullptr) return;

	// Call OnRelease using the moveable objects interface
	IMoveableObjectInterface::Execute_OnRelease(PhysicsHandle->GetGrabbedComponent()->GetOwner());

	// Finally, release the component
	PhysicsHandle->ReleaseComponent();
}

// Check if there is a grabbable object and return if there is
bool UGrabber::GetGrabbableInReach(FHitResult& OutHitResult, FRotator& OutOwnerRotation) const
{
	// Setup and set values for variables to store rotation and location
	FVector OwnerLocation;
	GetOwner()->GetActorEyesViewPoint(OwnerLocation, OutOwnerRotation);

	// Create a line trace between character and endpoint where character can reach
	FVector Start = OwnerLocation;
	FVector End = Start + GetForwardVector() * MaxGrabDistance + CameraOffsetVector;

	// Check for collisions with moveable actors
	FCollisionShape Sphere = FCollisionShape::MakeSphere(GrabRadius);
	return GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, ECC_GameTraceChannel1, Sphere, Params);
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