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
		MoveableObject = GetMoveableInRadius();

		// If near a fuseable object, glow green
		if (MoveableObject != nullptr) {
			DynamicMat->SetScalarParameterValue("Fuseable", 1.f);
		}
		// Otherwise glow red
		else {
			DynamicMat->SetScalarParameterValue("Fuseable", 0.f);
		}
	}
}

// Create and apply new material instance when obejct is grabbed
void AMoveableObject::OnGrab_Implementation()
{
	bIsGrabbed = true;

	// Create material overlay on grab to allow for updating of material parameters
	if (Mat != nullptr) {
		DynamicMat = UMaterialInstanceDynamic::Create(Mat, MeshComponent);
		DynamicMat->SetScalarParameterValue("Fuseable", 0.f);
		MeshComponent->SetOverlayMaterial(DynamicMat);
	}
}

// When the object is released, set its material back to the original
void AMoveableObject::OnRelease_Implementation()
{
	bIsGrabbed = false;

	// Remove overlay on release
	if (FuseMesh && FuseMesh->MeshComponent->GetOverlayMaterial() != nullptr) {
		RemoveMoveableItemMaterial(FuseMesh);
	}
	MeshComponent->SetOverlayMaterial(nullptr);

	if (MoveableObject != nullptr) {
		FuseMoveableObjects(MoveableObject);
	}
}

// Check for any overlapping moveable (fuseable) objects
AMoveableObject* AMoveableObject::GetMoveableInRadius()
{
	if (MeshComponent == nullptr) return nullptr;

	// Create sweep variables
	FVector TraceOrigin = MeshComponent->GetOwner()->GetActorLocation();
	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params(FName("SweepCheck"), false, MeshComponent->GetOwner());

	// Get all nearby objects
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceOrigin,
		TraceOrigin,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius),
		Params
	);

	// Search for and return moveable objects within nearby objects
	return GetMoveableItem(HitResults, TraceOrigin);
}

// Check if there is a grabbable object in sphere trace
AMoveableObject* AMoveableObject::GetMoveableItem(TArray<FHitResult> HitResults, FVector TraceOrigin)
{
	// Iterate over hit results
	for (const FHitResult& HitResult : HitResults) {
		AActor* HitActor = HitResult.GetActor();

		// Move to the next actor if current hit is not a valid actor or is not a moveable object
		if (!HitActor || !HitActor->IsA(AMoveableObject::StaticClass())) {
			continue;
		}

		// Store variables for individual line traces, checking if a nearby moveable object is blocked by other objects
		FVector TargetLocation = HitActor->GetActorLocation();
		FHitResult TestHit;

		// Line trace to see if there are any blocking objects
		bool bBlockedHit = GetWorld()->LineTraceSingleByChannel(
			TestHit,
			TraceOrigin,
			TargetLocation,
			ECC_Visibility,
			FCollisionQueryParams(FName("LOSCheck"), false, MeshComponent->GetOwner())
		);

		// If there are no blocking objects, update materials return moveable object
		if (!bBlockedHit || TestHit.GetActor() == HitActor) {
			AMoveableObject* NearbyMoveable = Cast<AMoveableObject>(HitActor);
			if (NearbyMoveable != FuseMesh || NearbyMoveable->MeshComponent->GetOverlayMaterial() == nullptr) {
				RemoveMoveableItemMaterial(FuseMesh);
				FuseMesh = NearbyMoveable;
				UpdateMoveableItemMaterial(NearbyMoveable);
			}
			return NearbyMoveable;
		}
	}

	// If no moveable objects are nearby, clear current fuse object's overlay material and return null
	if (FuseMesh && FuseMesh->MeshComponent->GetOverlayMaterial() != nullptr) {
		RemoveMoveableItemMaterial(FuseMesh);
	}
	return nullptr;
}

// Update material of nearby fuseable object
void AMoveableObject::UpdateMoveableItemMaterial(AMoveableObject* MoveableMesh)
{
	if (MoveableMesh->Mat != nullptr) {
		MoveableDynamicMat = UMaterialInstanceDynamic::Create(MoveableMesh->Mat, MeshComponent);
		MoveableDynamicMat->SetScalarParameterValue("Fuseable", 1.f);
		MoveableMesh->MeshComponent->SetOverlayMaterial(MoveableDynamicMat);
	}
}

// Remove material of nearby fuseable object
void AMoveableObject::RemoveMoveableItemMaterial(AMoveableObject* MoveableMesh)
{
	if (MoveableMesh) {
		MoveableMesh->MeshComponent->SetOverlayMaterial(nullptr);
	}
}

// Fuse with the nearest fuseable object
void AMoveableObject::FuseMoveableObjects(AMoveableObject* MoveableMesh)
{
	// Move nearby fuseable object to be aligned with held object
	FVector HeldCenter = MoveableMesh->GetActorLocation();
	FVector ClosestPoint;
	MeshComponent->GetClosestPointOnCollision(HeldCenter, ClosestPoint);
	MoveableMesh->SetActorLocation(ClosestPoint);

	// Create and setup a physics constraint
	UPhysicsConstraintComponent* PhysicsConstraint = NewObject<UPhysicsConstraintComponent>(MeshComponent->GetOwner());
	PhysicsConstraint->RegisterComponent();
	PhysicsConstraint->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	PhysicsConstraint->SetWorldLocation(MeshComponent->GetOwner()->GetActorLocation());
	PhysicsConstraint->SetConstrainedComponents(MeshComponent, NAME_None, MoveableMesh->MeshComponent, NAME_None);

	// Configure allowed motion and rotation
	PhysicsConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	PhysicsConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	PhysicsConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	PhysicsConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	PhysicsConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
	PhysicsConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);

	PhysicsConstraint->SetDisableCollision(true);
}