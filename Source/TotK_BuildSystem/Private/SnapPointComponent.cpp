#include "SnapPointComponent.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
USnapPointComponent::USnapPointComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void USnapPointComponent::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void USnapPointComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// Draw a debug sphere showing where the snapping point is located
void USnapPointComponent::DrawDebug()
{
	if (!GetWorld()) return;

	// Pick a color based on snap type
	FColor Color = FColor::White;

	switch (SnapType)
	{
	case ESnapType::BeamEnd:
		Color = FColor::Red;
		break;
	case ESnapType::BeamMiddle:
		Color = FColor::Orange;
		break;
	case ESnapType::BoardTop:
		Color = FColor::Green;
		break;
	case ESnapType::BoardSide:
		Color = FColor::Blue;
		break;
	case ESnapType::BoardFront:
		Color = FColor::Purple;
		break;
	case ESnapType::FanBottom:
		Color = FColor::Cyan;
		break;
	case ESnapType::WheelCenter:
		Color = FColor::Yellow;
		break;
	case ESnapType::WheelOuter:
		Color = FColor::Magenta;
		break;
	default:
		Color = FColor::White;
		break;
	}

	// Draw a small sphere at the snap point
	DrawDebugSphere(
		GetWorld(),
		GetComponentLocation(),
		5.0f,          // radius
		8,             // segments
		Color,
		false,         // persistent
		-1.f,          // lifetime
		0,             // depth priority
		0.5f           // line thickness
	);

	// Draw forward direction (useful for alignment debugging)
	DrawDebugLine(
		GetWorld(),
		GetComponentLocation(),
		GetComponentLocation() + GetForwardVector() * 10.f,
		Color,
		false,
		-1.f,
		0,
		0.5f
	);
}