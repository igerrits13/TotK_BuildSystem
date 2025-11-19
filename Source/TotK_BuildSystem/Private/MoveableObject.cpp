#include "MoveableObject.h"
#include "DrawDebugHelpers.h"
#include "SnapPointComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "../DebgugHelper.h"

// Sets default values
AMoveableObject::AMoveableObject()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Add the static mesh component as the root component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetNotifyRigidBodyCollision(true);
	MeshComponent->OnComponentHit.AddDynamic(this, &AMoveableObject::OnHit);
	RootComponent = MeshComponent;

	// Add the box collider for fusing objects
	FuseCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FuseCollisionBox"));
	FuseCollisionBox->SetupAttachment(MeshComponent);

	bIsFusing = false;
}

// Called when the game starts or when spawned
void AMoveableObject::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the set of fused objects and after this object has been created 
	FusedObjects.Add(this);
	ClosestFusedMoveableObject = this;

	// Initialize the array of snap points, storing all snap points created from the blue print
	GetComponents<USnapPointComponent>(SnapPoints);
}

// Called every frame
void AMoveableObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update the current velocities of the moveable object
	UpdateVelocities();

	// If an object is currently held, get the closest moveable object within its radius and update the overlay material based on if there is a nearby object
	if (bIsGrabbed && MeshComponent) {
		ClosestNearbyMoveableObject = GetClosestMoveableObjectInRadius();
		UpdateMoveableObjectMaterial(this, ClosestNearbyMoveableObject ? true : false);
	}

	// If a nearby moveable object exists, update the closest fusion points on the nearby moveable object and the currently held object
	if (ClosestNearbyMoveableObject) {
		UpdateCollisionPoints();

		// If two objects are currently fusing, interpolate the location of the previously held object to move towards the object it is fusing with, joining the two objects at the associated closest points
		if (bIsFusing) {
			InterpFusedObjects(DeltaTime);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging 
	// Draw the moveable object's collision box
	if (bDebugMode) {
		DrawDebugBox(
			GetWorld(),
			FuseCollisionBox->GetComponentLocation(),
			FuseCollisionBox->GetScaledBoxExtent(),
			FuseCollisionBox->GetComponentQuat(),
			FColor::Red,
			false,
			0,
			0,
			2.0f   // Line thickness
		);

		// Draw debug spheres for each snap point
		for (USnapPointComponent* Point : SnapPoints) {
			if (Point) {
				Point->DrawDebug();
			}
		}

		// For debugging - Draw collision points for fusing objects
		if (ClosestNearbyMoveableObject && bDebugMode) {
			DrawDebugPoint(
				GetWorld(),
				HeldClosestSnapPoint,
				15.f,
				FColor::Green,
				false,
				0.f
			);

			DrawDebugPoint(
				GetWorld(),
				OtherClosestSnapPoint,
				15.f,
				FColor::Orange,
				false,
				0.f
			);
		}
	}
	////////////////////////////////////////////////////////////////////////////////////
}

// Update the current velocities of the moveable object
void AMoveableObject::UpdateVelocities()
{
	PreviousVelocity = MeshComponent->GetPhysicsLinearVelocity();
	PreviousAngularVelocity = MeshComponent->GetPhysicsAngularVelocityInDegrees();
}

// Remove velocities on hit objects if they are another moveable object
void AMoveableObject::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	// Only change hit interaction if hitting another moveable object
	if (OtherActor->IsA(AMoveableObject::StaticClass())) {
		AMoveableObject* OtherMoveable = Cast<AMoveableObject>(OtherActor);

		// Keep any velocity that the moveable objects currently have without adding new velocities
		OtherComp->SetPhysicsLinearVelocity(OtherMoveable->PreviousVelocity);
		OtherComp->SetPhysicsAngularVelocityInDegrees(OtherMoveable->PreviousAngularVelocity);
	}
}

// When an object is grabbed, add an overlay material
void AMoveableObject::OnGrab_Implementation()
{
	bIsGrabbed = true;
	UpdateMoveableObjectMaterial(this, false);

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print out all fused objects and their physics constraint links
	if (bDebugMode) {
		// Print each object and the names of all moveable objects it is currently fused with
		for (AMoveableObject* Object : FusedObjects) {
			if (!Object) continue;

			FString ObjectName = Object->GetName();
			FString fusedNames;

			for (AMoveableObject* fused : Object->FusedObjects)
			{
				if (fused)
				{
					fusedNames += fused->GetName() + TEXT(", ");
				}
			}

			// Trim trailing comma
			if (fusedNames.EndsWith(TEXT(", ")))
			{
				fusedNames.LeftChopInline(2);
			}

			UE_LOG(LogTemp, Warning, TEXT("Fused Objects: %s -> %s"), *ObjectName, *fusedNames);
		}

		// Print out each object's constraint links
		TMap<AMoveableObject*, TArray<FString>> ConstraintMap;

		// Group constraints by owning object
		for (const FPhysicsConstraintLink& Link : PhysicsConstraintLinks)
		{
			if (!Link.Constraint) continue;

			auto AddConstraintTo = [&](AMoveableObject* Obj, AMoveableObject* Other)
				{
					if (!Obj || !Other) return;

					FString OtherName = Other->GetName();
					ConstraintMap.FindOrAdd(Obj).Add(OtherName);
				};

			AddConstraintTo(Link.ComponentA, Link.ComponentB);
			AddConstraintTo(Link.ComponentB, Link.ComponentA);
		}

		// Print the constraints
		for (const auto& Elem : ConstraintMap)
		{
			AMoveableObject* Obj = Elem.Key;
			const TArray<FString>& LinkedNames = Elem.Value;

			FString Line = Obj ? Obj->GetName() : TEXT("None");
			Line += TEXT(" -> ");

			for (const FString& Name : LinkedNames)
			{
				Line += Name + TEXT(", ");
			}

			// Trim trailing comma
			if (Line.EndsWith(TEXT(", ")))
			{
				Line.LeftChopInline(2);
			}

			UE_LOG(LogTemp, Warning, TEXT("Constraint Map: %s"), *Line);
		}
	}
	////////////////////////////////////////////////////////////////////////////////////
}

// When the object is released, remove overalay materials from all objects
void AMoveableObject::OnRelease_Implementation()
{
	bIsGrabbed = false;

	// Remove material from the currently held object and all of its fused objects
	RemoveMoveableObjectMaterial(this);

	// Remove material from nearby moveable object and all of its fused objects, if one exists. Then update the previous moveable object to be null
	if (PrevMoveableObject) {
		RemoveMoveableObjectMaterial(PrevMoveableObject);
		PrevMoveableObject = nullptr;
	}

	// If there is a nearby moveable object on release, fuse object groups together
	if (ClosestNearbyMoveableObject) {
		//FuseMoveableObjects(ClosestNearbyMoveableObject);
		bIsFusing = true;
	}

	// Set all fused object's velocities to zero
	RemoveObjectVelocity();
}

// Get the closest moveable object within the collision range
AMoveableObject* AMoveableObject::GetClosestMoveableObjectInRadius()
{
	// Initialize variable to store the current object and the currently closest object
	AMoveableObject* HitResultObject = nullptr;
	AMoveableObject* CurrClosestMoveableObject = nullptr;

	// Get the closest moveable object for each object in the currently held object's fused group
	for (AMoveableObject* FusedObject : FusedObjects) {
		// Do not check for collisions if the current object does not have a collision box
		if (!FusedObject->FuseCollisionBox) continue;

		// Get all overlapping actors with collision box
		TArray<AActor*> OverlapActors;
		FusedObject->FuseCollisionBox->GetOverlappingActors(OverlapActors, AMoveableObject::StaticClass());

		// Get current nearby moveable object
		HitResultObject = GetClosestMoveableObjectByActor(FusedObject, OverlapActors);

		// If there is no hit result, continue. Otherwise, add it to the array
		if (!HitResultObject) continue;

		// If there is no current closest moveable object, update it to be the most recent hit result
		if (!CurrClosestMoveableObject) {
			ClosestFusedMoveableObject = FusedObject;
			CurrClosestMoveableObject = HitResultObject;
		}

		// Otherwise, update the closest fused moveable object and the current moveable object to be the closest of the tested objects
		else CurrClosestMoveableObject = GetClosestMoveableofTwo(FusedObject, HitResultObject, ClosestFusedMoveableObject, CurrClosestMoveableObject);
	}

	// If the previous movable object is not the current moveable object, update prev movable object accordingly. Then update the overlay material and return
	if (PrevMoveableObject != CurrClosestMoveableObject && CurrClosestMoveableObject) {
		// If there was a previous moveable object, remove the overlay material from it, then update the previous moveable object to be the new closest and add an overlay material
		if (PrevMoveableObject && PrevMoveableObject->MeshComponent->GetOverlayMaterial()) {
			RemoveMoveableObjectMaterial(PrevMoveableObject);
		}
		PrevMoveableObject = CurrClosestMoveableObject;
		UpdateMoveableObjectMaterial(CurrClosestMoveableObject, true);
	}

	else if (!CurrClosestMoveableObject && PrevMoveableObject) {
		RemoveMoveableObjectMaterial(PrevMoveableObject);
		PrevMoveableObject = nullptr;
	}

	return CurrClosestMoveableObject;
}

// Get the closest moveable object for the current actor
AMoveableObject* AMoveableObject::GetClosestMoveableObjectByActor(AMoveableObject* FusedObject, TArray<AActor*> OverlapActors)
{
	// Initialize variables for line trace and to track nearby moveable object and closest moveable object
	FVector TraceOrigin = FusedObject->GetActorLocation();
	AMoveableObject* CurrMoveableObject = nullptr;
	AMoveableObject* CurrClosestMoveableObject = nullptr;

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print debug information for the current object
	if (bDebugMode) {
		//Debug::Print(TEXT("Checking closest for " + FusedObject->GetName()));
	}
	////////////////////////////////////////////////////////////////////////////////////

	// Iterate over all hit results to get the closest moveable object
	for (AActor* OverlapActor : OverlapActors) {
		////////////////////////////////////////////////////////////////////////////////////
		// For debugging - Print debug information for the current overlapping actor and draw grabbed object line trace
		if (bDebugMode) {
			DrawDebugPoint(
				GetWorld(),
				OverlapActor->GetActorLocation(),
				10.f,
				FColor::Red,
				false
			);

			DrawDebugLine(
				GetWorld(),
				TraceOrigin,
				OverlapActor->GetActorLocation(),
				FColor::Yellow,
				false
			);
		}
		////////////////////////////////////////////////////////////////////////////////////

		// Move to the next actor if current hit is not a valid actor, is not a moveable object, or actor is an already fused object
		if (!OverlapActor || !OverlapActor->IsA(AMoveableObject::StaticClass()) || FusedObject->FusedObjects.Contains(Cast<AMoveableObject>(OverlapActor))) continue;

		// Get the current actor moveable object
		CurrMoveableObject = CheckMoveableObjectTrace(Cast<AMoveableObject>(OverlapActor), FusedObject);

		// If the current actor has no valid hit, continue
		if (!CurrMoveableObject) continue;

		// If there is no current moveable object, update it to be the current moveable object being tested
		if (!CurrClosestMoveableObject) CurrClosestMoveableObject = CurrMoveableObject;

		// Otherwise, update moveable object to be the closest of the tested objects
		else CurrClosestMoveableObject = GetClosestMoveable(FusedObject, CurrClosestMoveableObject, CurrMoveableObject);
	}

	return CurrClosestMoveableObject;
}

// Run a line trace to check for a clear path between the hit actor and currently held object
AMoveableObject* AMoveableObject::CheckMoveableObjectTrace(AMoveableObject* NearbyMoveable, AMoveableObject* FusedObject)
{
	// Initialize variables used for line trace
	FVector TraceOrigin = FusedObject->GetActorLocation();
	FVector TargetLocation = NearbyMoveable->GetActorLocation();
	FHitResult TestHit;

	// Line trace from nearby moveable to  to see if there are any blocking objects
	bool bBlockedHit = GetWorld()->LineTraceSingleByChannel(
		TestHit,
		TraceOrigin,
		TargetLocation,
		ECC_Visibility,
		FCollisionQueryParams(FName("LOSCheck"), false, FusedObject->MeshComponent->GetOwner())
	);

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Draw grabbed object line trace
	if (bDebugMode) {
		DrawDebugPoint(
			GetWorld(),
			TestHit.ImpactPoint,
			10.f,
			FColor::Orange,
			false
		);

		DrawDebugLine(
			GetWorld(),
			TraceOrigin,
			TestHit.ImpactPoint,
			FColor::Green,
			false
		);
	}
	////////////////////////////////////////////////////////////////////////////////////

	// If line hit result is an already fused object, return nullptr
	if (FusedObject->FusedObjects.Contains(NearbyMoveable)) return nullptr;

	// If there are no blocking objects or the line trace hits the nearby moveable object, return moveable object
	if (!bBlockedHit || TestHit.GetActor() == NearbyMoveable) {
		return NearbyMoveable;
	}

	return nullptr;
}

// Update the closest collision points on the held object and the nearby fusion object
void AMoveableObject::UpdateCollisionPoints()
{
	// Get the closest collision points of both the held and nearby moveable object
	FVector HeldFuseObjectCenter = ClosestFusedMoveableObject->MeshComponent->GetOwner()->GetActorLocation();
	FVector HeldClosestFusionPoint, OtherClosestFusionPoint;
	ClosestNearbyMoveableObject->MeshComponent->GetClosestPointOnCollision(HeldFuseObjectCenter, OtherClosestFusionPoint);
	ClosestFusedMoveableObject->MeshComponent->GetClosestPointOnCollision(OtherClosestFusionPoint, HeldClosestFusionPoint);

	// From the closest collision point, get all possible snap points within a specified radius
	TArray<USnapPointComponent*> HeldSnapPoints = GetPossibleSnapPoints(HeldClosestFusionPoint, ClosestFusedMoveableObject);
	TArray<USnapPointComponent*> NearbySnapPoints = GetPossibleSnapPoints(OtherClosestFusionPoint, ClosestNearbyMoveableObject);

	// Get the closest snap point to the previously calculated collision point for the held object. If there is none, simply use the collision point itself
	USnapPointComponent* HeldClosestPoint = GetClosestObjectSnapPoint(HeldSnapPoints, HeldClosestFusionPoint);
	if (HeldClosestPoint) {
		HeldClosestSnapPoint = HeldClosestPoint->GetComponentLocation();
	}

	else {
		HeldClosestSnapPoint = HeldClosestFusionPoint;
	}

	// Get the closest snap point to the previously calculated collision point for the nearby object. If there is none, simply use the collision point itself
	USnapPointComponent* NearbyClosestPoint = GetClosestObjectSnapPoint(NearbySnapPoints, OtherClosestFusionPoint);
	if (NearbyClosestPoint) {
		OtherClosestSnapPoint = NearbyClosestPoint->GetComponentLocation();
	}

	else {
		OtherClosestSnapPoint = OtherClosestFusionPoint;
	}
}

// Get possible snap points
TArray<USnapPointComponent*> AMoveableObject::GetPossibleSnapPoints(FVector TestPoint, AMoveableObject* TestObject)
{
	TArray<USnapPointComponent*> TestSnapPoints;

	for (USnapPointComponent* SnapPoint : TestObject->SnapPoints) {
		if (!SnapPoint) continue;

		////////////////////////////////////////////////////////////////////////////////////
		// For debugging - Print the distance between objects
		if (bDebugMode) {
			Debug::Print(FString::Printf(TEXT("Testing %s:"), *SnapPoint->GetName()));
		}
		////////////////////////////////////////////////////////////////////////////////////

		if (FVector::DistSquared(SnapPoint->GetComponentLocation(), TestPoint) < SnapSearchRadius * SnapSearchRadius) {
			TestSnapPoints.Add(SnapPoint);
		}
	}

	return TestSnapPoints;
}

// Get the closest snap point on the held object
USnapPointComponent* AMoveableObject::GetClosestObjectSnapPoint(TArray<USnapPointComponent*> PossibleSnapPoints, FVector TestPoint)
{
	// Initialize a null return snap point
	USnapPointComponent* ClosestSnap = nullptr;

	// Iterate over all possible snap points, making sure to get the closest valid snap point
	for (USnapPointComponent* SnapPoint : PossibleSnapPoints) {
		if (!SnapPoint) continue;

		////////////////////////////////////////////////////////////////////////////////////
		// For debugging - Print the distance between objects
		if (bDebugMode) {
			Debug::Print(FString::Printf(TEXT("Testing %s:"), *SnapPoint->GetName()));
		}
		////////////////////////////////////////////////////////////////////////////////////

		/*if (!ClosestSnap) {
			ClosestSnap = SnapPoint;
			continue;
		}*/

		ClosestSnap = GetClosestVector(TestPoint, ClosestSnap, SnapPoint);
	}

	return ClosestSnap;
}

// Get the closest snap point to the current test point
USnapPointComponent* AMoveableObject::GetClosestVector(FVector TestPoint, USnapPointComponent* PointA, USnapPointComponent* PointB)
{
	// If an snap point is null, return the other
	if (!PointA) return PointB;
	if (!PointB) return PointA;

	// Compare the distance of each snap point to the test point and return the closest result
	float PointADist = GetVectorDistance(TestPoint, PointA->GetComponentLocation());
	float PointBDist = GetVectorDistance(TestPoint, PointB->GetComponentLocation());

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print the distance between objects
	if (bDebugMode) {
		/*Debug::Print(FString::Printf(TEXT("Distance A for %s to %s: %f"), *Held->GetName(), *ObjectA->GetName(), ObjectADist));
		Debug::Print(FString::Printf(TEXT("Distance B for %s to %s: %f"), *Held->GetName(), *ObjectB->GetName(), ObjectBDist));*/
	}
	////////////////////////////////////////////////////////////////////////////////////

	// If point A's distance to the test point is less than or equal to point B's distance, return point A
	if (PointADist <= PointBDist) {
		return PointA;
	}

	// Otherwise return point B
	else {
		return PointB;
	}
}

// Get the distance betwen two vectors
float AMoveableObject::GetVectorDistance(FVector PointA, FVector PointB)
{
	return FVector::Distance(PointA, PointB);
}

// Move objects being fused together via interpolation over time
void AMoveableObject::InterpFusedObjects(float DeltaTime)
{
	// Get the object offset from the center of the held object to the closest point of the held object and adjust the target location based on the offset
	FVector Offset = HeldClosestSnapPoint - ClosestFusedMoveableObject->MeshComponent->GetOwner()->GetActorLocation();
	FVector TargetActorLocation = OtherClosestSnapPoint - Offset;

	ClosestFusedMoveableObject->MeshComponent->GetOwner()->SetActorLocation(FMath::VInterpTo(ClosestFusedMoveableObject->MeshComponent->GetOwner()->GetActorLocation(), TargetActorLocation, DeltaTime, InterpSpeed));

	// Check the distance between closest points, once they are within the given tolerance, fusion has been completed and the closest nearby object no longer needs to be tracked
	float Distance = FVector::Dist(HeldClosestSnapPoint, OtherClosestSnapPoint);
	if (Distance <= FuseTolerance) {
		bIsFusing = false;
		UpdateConstraints(ClosestNearbyMoveableObject);
		ClosestNearbyMoveableObject = nullptr;
	}
}

// Get the closest object to what is currently held
AMoveableObject* AMoveableObject::GetClosestMoveable(AMoveableObject* Held, AMoveableObject* ObjectA, AMoveableObject* ObjectB)
{
	// If an object is null, return the other
	if (!ObjectA) return ObjectB;
	if (!ObjectB) return ObjectA;

	// Compare the distance of each object to the held object and return the closest result
	float ObjectADist = GetObjectDistance(Held, ObjectA);
	float ObjectBDist = GetObjectDistance(Held, ObjectB);

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print the distance between objects
	if (bDebugMode) {
		/*Debug::Print(FString::Printf(TEXT("Distance A for %s to %s: %f"), *Held->GetName(), *ObjectA->GetName(), ObjectADist));
		Debug::Print(FString::Printf(TEXT("Distance B for %s to %s: %f"), *Held->GetName(), *ObjectB->GetName(), ObjectBDist));*/
	}
	////////////////////////////////////////////////////////////////////////////////////

	// If object A's distance to the held object is less than or equal to object B's distance, return object A
	if (ObjectADist <= ObjectBDist) {
		return ObjectA;
	}

	// Otherwise return object B
	else {
		return ObjectB;
	}
}

// Get the closest objects between two object groups
AMoveableObject* AMoveableObject::GetClosestMoveableofTwo(AMoveableObject* TestFused, AMoveableObject* TestMoveable, AMoveableObject* CurrentBestFused, AMoveableObject* CurrentBestMoveable)
{
	// If an object is null, return the other
	if (!TestMoveable) return CurrentBestMoveable;
	if (!CurrentBestMoveable) return TestMoveable;

	// Compare the distance of each object to the held object and return the closest result
	float ObjectADist = GetObjectDistance(TestFused, TestMoveable);
	float ObjectBDist = GetObjectDistance(CurrentBestFused, CurrentBestMoveable);

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print the distance between objects
	if (bDebugMode) {
		/*Debug::Print(FString::Printf(TEXT("Distance A for %s to %s: %f"), *TestFused->GetName(), *TestMoveable->GetName(), ObjectADist));
		Debug::Print(FString::Printf(TEXT("Distance B for %s to %s: %f"), *CurrentBestFused->GetName(), *CurrentBestMoveable->GetName(), ObjectBDist));*/
	}
	////////////////////////////////////////////////////////////////////////////////////

	// If object A's distance to the held object is less than or equal to object B's distance, return object A
	if (ObjectADist <= ObjectBDist) {
		//RemoveMoveableObjectMaterial(currClosestMo)
		ClosestFusedMoveableObject = TestFused;
		return TestMoveable;
	}

	// Otherwise return object B
	else {
		return CurrentBestMoveable;
	}
}

// Get the distance betwen two moveable objects
float AMoveableObject::GetObjectDistance(AMoveableObject* MoveableA, AMoveableObject* MoveableB)
{
	return FVector::Distance(MoveableA->GetActorLocation(), MoveableB->GetActorLocation());
}

// Remove velocities from objects when dropping
void AMoveableObject::RemoveObjectVelocity()
{
	for (AMoveableObject* Object : FusedObjects) {
		Object->MeshComponent->WakeAllRigidBodies();
		Object->MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Object->MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
}

// Update material of nearby fuseable object and its currently fused object set
void AMoveableObject::UpdateMoveableObjectMaterial(AMoveableObject* MoveableObject, bool Fuseable)
{
	for (AMoveableObject* Object : MoveableObject->FusedObjects) {
		// If the object is not valid, move onto the next
		if (!Object || !Object->Mat || !Object->MeshComponent) continue;

		// Otherwise, create a dynamic material instance and set it as an overlay material
		Object->DynamicMat = UMaterialInstanceDynamic::Create(Object->Mat, Object->MeshComponent);
		Object->DynamicMat->SetScalarParameterValue("Fuseable", Fuseable ? 1.f : 0.f);
		Object->MeshComponent->SetOverlayMaterial(Object->DynamicMat);
	}
}

// Remove material of nearby fuseable object and its currently fused object set
void AMoveableObject::RemoveMoveableObjectMaterial(AMoveableObject* MoveableObject)
{
	for (AMoveableObject* Object : MoveableObject->FusedObjects) {
		// If the object is not valid, move onto the next
		if (!Object || !Object->Mat || !Object->MeshComponent) continue;

		// Otherwise remove the overlay material
		Object->MeshComponent->SetOverlayMaterial(nullptr);
		Object->DynamicMat = nullptr;
	}
}

// Update the physics constraints of the two objects being fused
void AMoveableObject::UpdateConstraints(AMoveableObject* MoveableObject)
{
	// Create and setup a physics constraint
	UPhysicsConstraintComponent* PhysicsConstraint = AddPhysicsConstraint(MoveableObject);

	// Create a custom link to add to the physics constraints array
	AddConstraintLink(PhysicsConstraint, MoveableObject);

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

				/*Debug::Print(FString::Printf(
					TEXT("Constraint: %s | CompA: %s (%s) | CompB: %s (%s) | Location: %s"),
					*ConstraintName,
					*CompAName, *ActorAName,
					*CompBName, *ActorBName,
					*WorldLocation.ToString()
				));*/
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////

	MergeMoveableObjects(MoveableObject);
}

// Create a new physics constraint on the closest moveable object within the held object's fused set to be used with the physics constraint link
UPhysicsConstraintComponent* AMoveableObject::AddPhysicsConstraint(AMoveableObject* MoveableObject)
{
	UPhysicsConstraintComponent* PhysicsConstraint = NewObject<UPhysicsConstraintComponent>(ClosestFusedMoveableObject->MeshComponent->GetOwner());
	PhysicsConstraint->RegisterComponent();
	PhysicsConstraint->AttachToComponent(ClosestFusedMoveableObject->RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	PhysicsConstraint->SetWorldLocation(ClosestFusedMoveableObject->MeshComponent->GetOwner()->GetActorLocation());
	PhysicsConstraint->SetConstrainedComponents(ClosestFusedMoveableObject->MeshComponent, NAME_None, MoveableObject->MeshComponent, NAME_None);

	// Configure allowed motion and rotation
	PhysicsConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	PhysicsConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	PhysicsConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	PhysicsConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	PhysicsConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
	PhysicsConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);

	// Do not allow fused objects to collide with each other
	PhysicsConstraint->SetDisableCollision(true);

	return PhysicsConstraint;
}

// Create a new constraint link and add it to both objects being fused
void AMoveableObject::AddConstraintLink(UPhysicsConstraintComponent* PhysicsConstraint, AMoveableObject* MoveableObject)
{
	FPhysicsConstraintLink NewLink;
	NewLink.Constraint = PhysicsConstraint;
	NewLink.ComponentA = ClosestFusedMoveableObject;
	NewLink.ComponentB = MoveableObject;
	ClosestFusedMoveableObject->PhysicsConstraintLinks.Add(NewLink);
	MoveableObject->PhysicsConstraintLinks.Add(NewLink);
}

// Merge the fused object sets of the currently held object and the one it is fusing with
void AMoveableObject::MergeMoveableObjects(AMoveableObject* MoveableObject)
{
	// Initialize a new set to hold all fused objects
	TSet<AMoveableObject*> MergedObjects;

	// Build set from currently existing fused object's sets of the held object and fusing object
	for (AMoveableObject* Object : ClosestFusedMoveableObject->FusedObjects) {
		if (Object) {
			MergedObjects.Add(Object);
		}
	}

	for (AMoveableObject* Object : MoveableObject->FusedObjects) {
		if (Object) {
			MergedObjects.Add(Object);
		}
	}

	// Update fused object of every object in the merged group
	for (AMoveableObject* Object : MergedObjects) {
		if (Object) {
			Object->FusedObjects = MergedObjects;
		}
	}
}

// Split the fused object sets of the currently held object through moveable object interface
void AMoveableObject::SplitMoveableObjects_Implementation()
{
	// Remove all physics constraints from the held object
	RemovePhysicsLink();

	// Clear the fused object set of each object except for itself and update its overlay material to be null
	for (AMoveableObject* Object : FusedObjects) {
		if (Object && Object != this) {
			Object->FusedObjects.Empty();
			Object->FusedObjects.Add(Object);
			Object->MeshComponent->SetOverlayMaterial(nullptr);
			Object->DynamicMat = nullptr;
		}
	}

	// Rebuild the fused object sets based on their physics links
	UpdateFusedSet();

	// Clear physics constraint links of held object
	PhysicsConstraintLinks.Empty();

	// Remove velocity from all previously fused objects to drop them
	RemoveObjectVelocity();

	// Finally, clear the fused objects set except for the object itself
	FusedObjects.Empty();
	FusedObjects.Add(this);
}

// Remove all physics constraints from the held object
void AMoveableObject::RemovePhysicsLink()
{
	for (const FPhysicsConstraintLink& Link : PhysicsConstraintLinks) {
		if (Link.Constraint) {
			// Re-enable collision on both objects
			Link.ComponentA->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Link.ComponentB->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			// If components are the same, simply destroy the constraint
			if (Link.ComponentA == Link.ComponentB) {
				Link.Constraint->DestroyComponent();
				continue;
			}

			// Remove the link from the physics constraint links of the non-held object to not interfere with this.PhysicsConstraintLinks. This will be cleared later
			if (Link.ComponentA == this) {
				Link.ComponentB->PhysicsConstraintLinks.Remove(Link);
			}

			else {
				Link.ComponentA->PhysicsConstraintLinks.Remove(Link);
			}

			// Destroy the constraint between the two objects
			Link.Constraint->DestroyComponent();
		}
	}
}

// Update fused object sets based on their physics links
void AMoveableObject::UpdateFusedSet()
{
	for (AMoveableObject* Object : FusedObjects) {
		if (Object && Object != this) {
			// Initialize a new set to store all fused objects from each link component
			TSet<AMoveableObject*> MergedSet;

			// For each object, iterate over all their physics links adding all moveable ojbects within each component's fused object set
			for (FPhysicsConstraintLink& Link : Object->PhysicsConstraintLinks) {
				if (Link.ComponentA) {
					MergedSet.Append(Link.ComponentA->FusedObjects);
				}

				if (Link.ComponentB) {
					MergedSet.Append(Link.ComponentB->FusedObjects);
				}

				// Then iterate over every object in the new merged set, setting it's fused object set to include all connected objects
				for (AMoveableObject* ObjectMerged : MergedSet) {
					ObjectMerged->FusedObjects = MergedSet;
				}
			}
		}
	}
}