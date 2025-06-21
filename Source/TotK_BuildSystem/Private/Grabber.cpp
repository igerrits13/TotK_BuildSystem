// Fill out your copyright notice in the Description page of Project Settings.


#include "Grabber.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

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
	PhysicsHandle->InterpolationSpeed = 10.f;

	// Vector for offsetting the height of held objects caused by third-person camera
	CameraOffsetVector = FVector(0.f, 0.f, GetOwner()->GetSimpleCollisionHalfHeight() * 1.5);
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
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Start, GetOwner()->GetActorLocation());
	FVector Direction = LookAtRot.Vector();
	FVector End = Start + (Direction * 100.0f); 
	DrawDebugDirectionalArrow(GetWorld(), Start, End, 10, FColor::Purple, false, 0.f, 0, 2.f);

	// Otherwise, set the held objects current location and rotation
	FVector TargetLocation = GetOwner()->GetActorLocation() + GetForwardVector() * CurrentHoldDistance + CameraOffsetVector;
	HeldRotation = UKismetMathLibrary::FindLookAtRotation(PhysicsHandle->GetGrabbedComponent()->GetComponentLocation(), GetOwner()->GetActorLocation());
	PhysicsHandle->SetTargetLocationAndRotation(TargetLocation, HeldRotation + AdjustedRotation);

	// Initialize variable to store hit result from out parameter and rotation in respect to where the object being picked up is at
	FVector OwnerLocation;
	FRotator OwnerRotation;

	GetOwner()->GetActorEyesViewPoint(OwnerLocation, OwnerRotation);
	GetOwner()->SetActorRotation(FRotator(0,OwnerRotation.Yaw,0));
}

// Grab item if able
void UGrabber::Grab()
{
	// Exit if there is no valid owner or physics handle
	if (!GetOwner() || PhysicsHandle == nullptr) return;

	// Initialize variable to store hit result and rotation in respect to where the object being picked up is at
	FHitResult HitResult;
	FRotator OwnerRotation;

	// If there is a valid hit
	if (GetGrabbableInReach(HitResult, OwnerRotation)) {
		// Rotate the player towrards the object being picked up
		GetOwner()->SetActorRotation(OwnerRotation);

		// Get the component being grabbed and wake up rigid bodies
		UPrimitiveComponent* HitComponent = HitResult.GetComponent();
		HitComponent->WakeAllRigidBodies();

		// Find information for setting the held objects location and rotation and reset adjusted rotation
		FVector PlayerLocation = GetOwner()->GetActorLocation();
		FVector ObjectLocation = HitComponent->GetComponentLocation();
		CurrentHoldDistance = FVector::Dist(PlayerLocation, ObjectLocation);
		AdjustedRotation = FRotator::ZeroRotator;

		// Grab the object with its current location and rotation
		PhysicsHandle->GrabComponentAtLocationWithRotation(
			HitComponent,
			NAME_None,
			ObjectLocation,
			HitComponent->GetComponentRotation()
		);
	}
}

// Release grabbed item
void UGrabber::Release()
{
	// Exit if there is no valid physics handle or no held object
	if (PhysicsHandle == nullptr ||  PhysicsHandle->GetGrabbedComponent() == nullptr) return;

	// Otherwise, wake up the rigid body, remove any velocity from the object and drop the object straight down
	PhysicsHandle->GetGrabbedComponent()->WakeAllRigidBodies();
	PhysicsHandle->GetGrabbedComponent()->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsHandle->GetGrabbedComponent()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
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
	return GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, ECC_GameTraceChannel1, Sphere);
}

// Check if the player is currently holding an item
bool UGrabber::IsHoldingObject()
{
	return PhysicsHandle->GetGrabbedComponent() != nullptr;
}

// Rotate the currently held object to the left
void UGrabber::RotateLeft()
{
	AdjustedRotation += FRotator(0.f, RotationDegrees, 0.f);
}

// Rotate the currently held object to the right
void UGrabber::RotateRight()
{
	AdjustedRotation += FRotator(0.f, -RotationDegrees, 0.f);
}

// Rotate the currently held object up
void UGrabber::RotateUp()
{
	AdjustedRotation += FRotator(RotationDegrees, 0.f, 0.f);
	//AdjustedRotation += UKismetMathLibrary::RotatorFromAxisAndAngle(-GetOwner()->GetActorForwardVector(), RotationDegrees);
}

// Rotate the currently held object down
void UGrabber::RotateDown()
{
	AdjustedRotation += FRotator(-RotationDegrees, 0.f, 0.f);
}