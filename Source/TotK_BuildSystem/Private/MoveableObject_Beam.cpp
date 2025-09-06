// Fill out your copyright notice in the Description page of Project Settings.


#include "MoveableObject_Beam.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/StaticMeshComponent.h"

#include "../DebgugHelper.h"

// Called every frame
void AMoveableObject_Beam::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print out all physics constraints on the current moveable object
	if (ClosestNearbyMoveableObject != nullptr && bDebugMode) {
		DrawDebugPoint(
			GetWorld(),
			HeldClosestFusionPoint,
			15.f,
			FColor::Orange,
			false, // PersistentLines (true to keep it visible even after duration)
			0.f
		);

		DrawDebugPoint(
			GetWorld(),
			OtherClosestFusionPoint,
			15.f,
			FColor::Yellow,
			false, // PersistentLines (true to keep it visible even after duration)
			0.f
		);
	}
	////////////////////////////////////////////////////////////////////////////////////

	// If a nearby moveable object exists, update the losest fusion points on that and the current held object
	if (ClosestNearbyMoveableObject) {
		UpdateCollisionPoints();
	}

	// If two objects are currently fusing, interpolate the location of the previously held object to move towards the object it is fusing with, joining the two objects at the associated closest points
	if (ClosestNearbyMoveableObject != nullptr && bIsFusing) {
		InterpFusedObjects(DeltaTime);
	}
}

// Fuse current object group with the nearest fuseable object
void AMoveableObject_Beam::FuseMoveableObjects(AMoveableObject* MoveableObject)
{
	Debug::Print(TEXT("Running from child"));

	// Calculate closest points between objects
	FVector HeldFuseObjectCenter = MeshComponent->GetOwner()->GetActorLocation();
	MoveableObject->MeshComponent->GetClosestPointOnCollision(HeldFuseObjectCenter, OtherClosestFusionPoint);

	MeshComponent->GetClosestPointOnCollision(OtherClosestFusionPoint, HeldClosestFusionPoint);

	// Begin fusing in tick function
	bIsFusing = true;
}

// Update the physics constraints of the two objects being fused
void AMoveableObject_Beam::UpdateConstraints(AMoveableObject* MoveableObject)
{
	// Create and setup a physics constraint
	UPhysicsConstraintComponent* PhysicsConstraint = NewObject<UPhysicsConstraintComponent>(MeshComponent->GetOwner());
	PhysicsConstraint->RegisterComponent();
	PhysicsConstraint->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	PhysicsConstraint->SetWorldLocation(MeshComponent->GetOwner()->GetActorLocation());
	PhysicsConstraint->SetConstrainedComponents(MeshComponent, NAME_None, MoveableObject->MeshComponent, NAME_None);

	// Configure allowed motion and rotation
	PhysicsConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	PhysicsConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	PhysicsConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	PhysicsConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	PhysicsConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
	PhysicsConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);

	PhysicsConstraint->SetDisableCollision(true);

	// Create a custom link to add to the physics constraints array
	FPhysicsConstraintLink NewLink;
	NewLink.Constraint = PhysicsConstraint;
	NewLink.ComponentA = this;
	NewLink.ComponentB = MoveableObject;
	PhysicsConstraintLinks.Add(NewLink);
	MoveableObject->AddConstraintLink(NewLink);

	//////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print out all physics constraints on the current moveable object
	if (bDebugMode) {
		for (const FPhysicsConstraintLink& Link : PhysicsConstraintLinks)
		{
			if (Link.Constraint)
			{
				FString ConstraintName = Link.Constraint->GetName();

				FString CompAName = Link.ComponentA ? Link.ComponentA->GetName() : TEXT("None");
				FString CompBName = Link.ComponentB ? Link.ComponentB->GetName() : TEXT("None");

				FString ActorAName = Link.ComponentA && Link.ComponentA->GetOwner() ? Link.ComponentA->GetOwner()->GetName() : TEXT("None");
				FString ActorBName = Link.ComponentB && Link.ComponentB->GetOwner() ? Link.ComponentB->GetOwner()->GetName() : TEXT("None");

				FVector WorldLocation = Link.Constraint->GetComponentLocation();

				Debug::Print(FString::Printf(
					TEXT("Constraint: %s | CompA: %s (%s) | CompB: %s (%s) | Location: %s"),
					*ConstraintName,
					*CompAName, *ActorAName,
					*CompBName, *ActorBName,
					*WorldLocation.ToString()
				));
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////

	MergeMoveableObjects(MoveableObject);
}

// Update the closest collision points on the held object and the nearby fusion object
void AMoveableObject_Beam::UpdateCollisionPoints()
{
	FVector HeldFuseObjectCenter = MeshComponent->GetOwner()->GetActorLocation();
	ClosestNearbyMoveableObject->MeshComponent->GetClosestPointOnCollision(HeldFuseObjectCenter, OtherClosestFusionPoint);

	MeshComponent->GetClosestPointOnCollision(OtherClosestFusionPoint, HeldClosestFusionPoint);
}

// Move objects being fused together via interpolation over time
void AMoveableObject_Beam::InterpFusedObjects(float DeltaTime)
{
	// Get the object offset from the center of the held object to the closest point of the held object and adjust the target location based on the offset
	FVector Offset = HeldClosestFusionPoint - MeshComponent->GetOwner()->GetActorLocation();
	FVector TargetActorLocation = OtherClosestFusionPoint - Offset;

	Debug::Print(TEXT("Interping"));
	MeshComponent->GetOwner()->SetActorLocation(FMath::VInterpTo(MeshComponent->GetOwner()->GetActorLocation(), TargetActorLocation, DeltaTime, InterpSpeed));

	// Check the distance between closest points, once they are within the given tolerance, fusion has been completed
	float Distance = FVector::Dist(HeldClosestFusionPoint, OtherClosestFusionPoint);
	if (Distance <= FuseTolerance) {
		Debug::Print(TEXT("Done Interping"));
		bIsFusing = false;
		UpdateConstraints(ClosestNearbyMoveableObject);
	}
}
