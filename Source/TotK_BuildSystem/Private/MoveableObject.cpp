#include "MoveableObject.h"
#include "DrawDebugHelpers.h"

#include "../DebgugHelper.h"

// Sets default values
AMoveableObject::AMoveableObject()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Add the static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetNotifyRigidBodyCollision(true);
	MeshComponent->OnComponentHit.AddDynamic(this, &AMoveableObject::OnHit);
	RootComponent = MeshComponent;

	// Add the box collider for fusing objects
	FuseCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FuseCollisionBox"));
	FuseCollisionBox->SetupAttachment(MeshComponent);

	// Initialize the set of fused objects and physics constraints
	FusedObjects.Add(this);
	PhysicsConstraintLinks.Empty();
}

// Called when the game starts or when spawned
void AMoveableObject::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMoveableObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Store the current velocities of the moveable object
	PreviousVelocity = MeshComponent->GetPhysicsLinearVelocity();
	PreviousAngularVelocity = MeshComponent->GetPhysicsAngularVelocityInDegrees();

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Draw collision box
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
	}
	////////////////////////////////////////////////////////////////////////////////////

	if (bIsGrabbed) {
		// Get the closest moveable object if one exists
		CurrentMoveableObject = GetMoveableInRadius();

		// Update material for all fused objects based on if there is a nearby moveable object or not
		UpdateMoveableObjectMaterial(this, CurrentMoveableObject ? true : false);
	}
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

	////////////////////////////////////////////////////////////////////////////////////
	// For debugging - Print out all fused objects
	if (bDebugMode) {
		// Update material for all fused objects of the currently grabbed object
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
	}

	if (bDebugMode) {
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

		// Print out each object's constraint links
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

	UpdateMoveableObjectMaterial(this, false);
}

// When the object is released, remove overalay material
void AMoveableObject::OnRelease_Implementation()
{
	bIsGrabbed = false;

	// Remove material from nearby moveable object and all its fused objects, if one exists
	if (PrevMoveableObject) {
		RemoveMoveableObjectMaterial(PrevMoveableObject);
	}

	// Remove material from the currently held object and all of its fused objects
	RemoveMoveableObjectMaterial(this);

	// If there is a nearby moveable object on release, fuse object groups together
	if (CurrentMoveableObject != nullptr) {
		FuseMoveableObjects(CurrentMoveableObject);
	}

	// Set all fused object's velocities to zero
	RemoveObjectVelocity();

	// Reset previous moveable object
	PrevMoveableObject = nullptr;
}

// Check for any nearby moveable objects for each object in the currently held object's fused group
AMoveableObject* AMoveableObject::GetMoveableInRadius()
{
	if (MeshComponent == nullptr) return nullptr;

	for (AMoveableObject* Object : FusedObjects) {
		if (!Object->FuseCollisionBox) continue;

		// Initialize variables used for sphere sweep
		FVector TraceOrigin = Object->GetActorLocation();
		TArray<AActor*> OverlapActors;

		Object->FuseCollisionBox->GetOverlappingActors(OverlapActors, AMoveableObject::StaticClass());

		// Get closest moveable object
		AMoveableObject* HitResultObject = GetMoveableObject(OverlapActors, TraceOrigin);

		if (!HitResultObject) continue;

		return HitResultObject;
	}

	return nullptr;
}

// Get closest moveable object
AMoveableObject* AMoveableObject::GetMoveableObject(TArray<AActor*> OverlapActors, FVector TraceOrigin)
{
	// Initialize variable to track nearby moveable object
	AMoveableObject* MoveableObject;

	// Iterate over all hit results
	for (AActor* Actor : OverlapActors) {
		////////////////////////////////////////////////////////////////////////////////////
		// For debugging - Draw grabbed object line trace
		if (bDebugMode) {
			DrawDebugPoint(
				GetWorld(),
				Actor->GetActorLocation(),
				10.f,
				FColor::Red,
				false
			);

			DrawDebugLine(
				GetWorld(),
				TraceOrigin,
				Actor->GetActorLocation(),
				FColor::Yellow,
				false
			);
		}
		////////////////////////////////////////////////////////////////////////////////////

		// Move to the next actor if current hit is not a valid actor, is not a moveable object or is its own collision box
		if (!Actor || !Actor->IsA(AMoveableObject::StaticClass()) || FusedObjects.Contains(Cast<AMoveableObject>(Actor))) continue;

		// Update moveable object based on line trace. If null, continue, otherwise return the nearby moveable object
		MoveableObject = CheckMoveableObjectTrace(Actor, TraceOrigin);

		if (MoveableObject == nullptr) continue;

		return MoveableObject;
	}

	// If no moveable objects are nearby, clear current fuse object's overlay material if one exists and return null
	if (PrevMoveableObject && PrevMoveableObject->MeshComponent->GetOverlayMaterial() != nullptr) {
		RemoveMoveableObjectMaterial(PrevMoveableObject);
	}

	return nullptr;
}

// Run a line trace to check if nearby moveable object is a valid hit
AMoveableObject* AMoveableObject::CheckMoveableObjectTrace(AActor* HitActor, FVector TraceOrigin)
{
	// Initialize variables used for line trace
	FVector TargetLocation = HitActor->GetActorLocation();
	FHitResult TestHit;
	AMoveableObject* NearbyMoveable = Cast<AMoveableObject>(HitActor);

	// Line trace to see if there are any blocking objects
	bool bBlockedHit = GetWorld()->LineTraceSingleByChannel(
		TestHit,
		TargetLocation,
		TraceOrigin,
		ECC_Visibility,
		FCollisionQueryParams(FName("LOSCheck"), false, MeshComponent->GetOwner())
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
	if (FusedObjects.Contains(NearbyMoveable)) return nullptr;

	// If there are no blocking objects or no objects between actors, update materials if needed and return moveable object
	if (!bBlockedHit || TestHit.GetActor() == HitActor) {
		// If there is a new nearby moveable object or the nearby moveable does not have a overlay material, update the previous moveable object and add an overlay material
		if (NearbyMoveable != PrevMoveableObject || NearbyMoveable->MeshComponent->GetOverlayMaterial() == nullptr) {
			// If the previous nearby moveable object still has a overlay material, remove it
			if (PrevMoveableObject && PrevMoveableObject->MeshComponent->GetOverlayMaterial() != nullptr) {
				RemoveMoveableObjectMaterial(PrevMoveableObject);
			}
			PrevMoveableObject = NearbyMoveable;
			// Update materials for all fused objects of the nearby moveable object
			UpdateMoveableObjectMaterial(PrevMoveableObject, true);
		}
		return PrevMoveableObject;
	}
	return nullptr;
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
		if (!Object || !Object->Mat || !Object->MeshComponent) return;
		Object->DynamicMat = UMaterialInstanceDynamic::Create(Object->Mat, Object->MeshComponent);
		Object->DynamicMat->SetScalarParameterValue("Fuseable", Fuseable ? 1.f : 0.f);
		Object->MeshComponent->SetOverlayMaterial(Object->DynamicMat);
	}
}

// Remove material of nearby fuseable object and its currently fused object set
void AMoveableObject::RemoveMoveableObjectMaterial(AMoveableObject* MoveableObject)
{
	for (AMoveableObject* Object : MoveableObject->FusedObjects) {
		if (!Object || !Object->Mat || !Object->MeshComponent) return;
		Object->MeshComponent->SetOverlayMaterial(nullptr);
		Object->DynamicMat = nullptr;
	}
}

// Fuse current object group with the nearest fuseable object
void AMoveableObject::FuseMoveableObjects(AMoveableObject* MoveableObject)
{
	// Move nearby fuseable object to be aligned with held object
	FVector FuseObjectCenter = MoveableObject->GetActorLocation();
	FVector ClosestPoint;
	MeshComponent->GetClosestPointOnCollision(FuseObjectCenter, ClosestPoint);
	MoveableObject->SetActorLocation(ClosestPoint);

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

	////////////////////////////////////////////////////////////////////////////////////
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
	////////////////////////////////////////////////////////////////////////////////////

	MergeMoveableObjects(MoveableObject);
}

// Merge the fused object sets of the currently held object and the one it is fusing with
void AMoveableObject::MergeMoveableObjects(AMoveableObject* MoveableObject)
{
	// Initialize a new set to hold all fused objects
	TSet<AMoveableObject*> MergedObjects;

	// Build set from currently existing fused object's sets of the held object and fusing object
	for (AMoveableObject* Object : FusedObjects)
	{
		if (Object) {
			MergedObjects.Add(Object);
		}
	}

	for (AMoveableObject* Object : MoveableObject->FusedObjects)
	{
		if (Object) {
			MergedObjects.Add(Object);
		}
	}

	// Update fused object of every object in the merged group
	for (AMoveableObject* Object : MergedObjects)
	{
		if (Object)
		{
			Object->FusedObjects = MergedObjects;
		}
	}
}

// Helper function to add constraint links
void AMoveableObject::AddConstraintLink(const FPhysicsConstraintLink& Link)
{
	PhysicsConstraintLinks.Add(Link);
}

// Split the fused object sets of the currently held object through moveable object interface
void AMoveableObject::SplitMoveableObjects_Implementation()
{
	/*for (const FPhysicsConstraintLink& Link : PhysicsConstraintLinks) {
		if (Link.Constraint) {
			Link.ComponentA->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Link.ComponentB->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			if (Link.ComponentA == Link.ComponentB) {
				Link.Constraint->DestroyComponent();
				continue;
			}
			if (Link.ComponentA == this) {
				Link.ComponentB->PhysicsConstraintLinks.Remove(Link);
			}

			else {
				Link.ComponentA->PhysicsConstraintLinks.Remove(Link);
			}

			Link.Constraint->DestroyComponent();
		}
	}*/

	// Remove all physics constraints from the held object
	RemovePhysicsLink();

	// Clear all fused object sets from all previously fused objects, keeping only the object itself and remove overlay material
	for (AMoveableObject* Object : FusedObjects) {
		if (Object && Object != this) {
			Object->MeshComponent->SetOverlayMaterial(nullptr);
			Object->DynamicMat = nullptr;
		}
	}

	// Clear the fused objects sets, leaving only the object itself
	for (AMoveableObject* Object : FusedObjects) {
		if (Object && Object != this) {
			Object->FusedObjects.Empty();
			Object->FusedObjects.Add(Object);
		}
	}

	// Update fused object sets based on their physics links
	UpdateFusedSet();

	// Clear physics constraint links of held object
	PhysicsConstraintLinks.Empty();

	// Remove the current object from all fused object lists
	RemoveObjectVelocity();

	// Clear fused objects set
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

			// Remove the link from the physics constraint links of the non-held object
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