#include "MoveableObject.h"

#include "../DebgugHelper.h"

// Sets default values
AMoveableObject::AMoveableObject()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Add the static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
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
		AMoveableObject* MoveableObject = GetMoveableInRadius();

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

	if (Mat != nullptr) {
		//DynamicMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
		DynamicMat = UMaterialInstanceDynamic::Create(Mat, MeshComponent);
		DynamicMat->SetScalarParameterValue("Fuseable", 0.f);
		//MeshComponent->SetMaterial(1, DynamicMat);
		MeshComponent->SetOverlayMaterial(DynamicMat);
	}
}

// When the object is released, set its material back to the original
void AMoveableObject::OnRelease_Implementation()
{
	bIsGrabbed = false;

	//MeshComponent->SetMaterial(1, nullptr);
	MeshComponent->SetOverlayMaterial(nullptr);
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

		// If there are no blocking objects, return moveable object
		if (!bBlockedHit || TestHit.GetActor() == HitActor) {
			return Cast<AMoveableObject>(HitActor);
		}
	}

	// Otherwise return nullptr
	return nullptr;
}