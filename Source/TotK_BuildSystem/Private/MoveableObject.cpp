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

	// Store the default material for setting on release
	DefaultMat = MeshComponent->GetMaterial(0);
}

// Called every frame
void AMoveableObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Create and apply new material instance when obejct is grabbed
void AMoveableObject::OnGrab_Implementation()
{
	if (DefaultMat != nullptr) {
		DynamicMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
		DynamicMat->SetScalarParameterValue("Intensity", 10.f);
		DynamicMat->SetScalarParameterValue("Fuseable", 0.f);
	}
}

// When the object is released, set its material back to the original
void AMoveableObject::OnRelease_Implementation()
{
	if (DefaultMat != nullptr) {
		MeshComponent->SetMaterial(0, DefaultMat);
	}
}