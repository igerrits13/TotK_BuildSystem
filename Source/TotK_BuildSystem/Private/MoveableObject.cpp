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
	RootComponent = MeshComponent;

	// Initialize the set of fused objects
	FusedObjects.Add(this);
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

	if (bIsGrabbed) {
		// Get the closest moveable object if one exists
		CurrentMoveableObject = GetMoveableInRadius();

		// Update material for all fused objects based on if there is a nearby moveable object or not
		UpdateMoveableObjectMaterial(this, CurrentMoveableObject ? true : false);
	}
}

// When an object is grabbed, add an overlay material
void AMoveableObject::OnGrab_Implementation()
{
	bIsGrabbed = true;

	// Update material for all fused objects of the currently grabbed object
	for (AMoveableObject* Object : FusedObjects) {
		if (!Object) continue;

		////////////////////////////////////////////////////////////////////////////////////
		// For debugging - Print out all fused objects
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

		UE_LOG(LogTemp, Warning, TEXT("%s -> %s"), *ObjectName, *fusedNames);
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

	// Reset previous moveable object
	PrevMoveableObject = nullptr;
}

// Check for any nearby moveable objects
AMoveableObject* AMoveableObject::GetMoveableInRadius()
{
	if (MeshComponent == nullptr) return nullptr;

	// Initialize variables used for sphere sweep
	FVector TraceOrigin = MeshComponent->GetOwner()->GetActorLocation();
	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params(FName("SweepCheck"), false, MeshComponent->GetOwner());

	// Get all nearby objects within sphere sweep
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceOrigin,
		TraceOrigin,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius),
		Params
	);

	// Get closest moveable object
	return GetMoveableObject(HitResults, TraceOrigin);
}

// Get closest moveable object
AMoveableObject* AMoveableObject::GetMoveableObject(TArray<FHitResult> HitResults, FVector TraceOrigin)
{
	// Initialize variable to track nearby moveable object
	AMoveableObject* MoveableObject;

	// Iterate over all hit results
	for (const FHitResult& HitResult : HitResults) {
		AActor* HitActor = HitResult.GetActor();

		// Move to the next actor if current hit is not a valid actor or is not a moveable object
		if (!HitActor || !HitActor->IsA(AMoveableObject::StaticClass())) continue;

		// Update moveable object based on line trace. If null, continue, otherwise return the nearby moveable object
		MoveableObject = CheckMoveableObjectTrace(HitActor, TraceOrigin);

		if (MoveableObject == nullptr) continue;

		return MoveableObject;
	}

	// If no moveable objects are nearby, clear current fuse object's overlay material if one exists and return null
	if (PrevMoveableObject && PrevMoveableObject->MeshComponent->GetOverlayMaterial() != nullptr) {
		RemoveMoveableObjectMaterial(PrevMoveableObject);
	}

	return nullptr;
}


AMoveableObject* AMoveableObject::CheckMoveableObjectTrace(AActor* HitActor, FVector TraceOrigin)
{
	// Initialize variables used for line trace
	FVector TargetLocation = HitActor->GetActorLocation();
	FHitResult TestHit;
	AMoveableObject* NearbyMoveable = Cast<AMoveableObject>(HitActor);

	// Line trace to see if there are any blocking objects
	bool bBlockedHit = GetWorld()->LineTraceSingleByChannel(
		TestHit,
		TraceOrigin,
		TargetLocation,
		ECC_Visibility,
		FCollisionQueryParams(FName("LOSCheck"), false, MeshComponent->GetOwner())
	);

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
	FVector HeldCenter = MoveableObject->GetActorLocation();
	FVector ClosestPoint;
	MeshComponent->GetClosestPointOnCollision(HeldCenter, ClosestPoint);
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