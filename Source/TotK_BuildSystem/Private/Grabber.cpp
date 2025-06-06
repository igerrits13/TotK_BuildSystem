// Fill out your copyright notice in the Description page of Project Settings.


#include "Grabber.h"
#include "Engine/World.h"
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

	// ...
	
}


// Called every frame
void UGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Exit if there is no valid owner
	if (!GetOwner()) return;

	// Setup variables to store rotation and location
	FVector OwnerLocation;
	FRotator OwnerRotation;
	GetOwner()->GetActorEyesViewPoint(OwnerLocation, OwnerRotation);

	// Create a line trace between character and endpoint where character can reach
	FVector Start = OwnerLocation;
	FVector End = Start + GetForwardVector() * MaxGrabDistance;

	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false);

	// Check for collisions with moveable actors
	FCollisionShape Sphere = FCollisionShape::MakeSphere(GrabRadius);
	FHitResult HitResult;
	bool HasHit = GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, ECC_GameTraceChannel1, Sphere);

	if (HasHit) {
		AActor* HitActor = HitResult.GetActor();
		UE_LOG(LogTemp, Display, TEXT("Hit: %s"), *HitActor->GetActorNameOrLabel());
	}

	else {
		UE_LOG(LogTemp, Display, TEXT("No hit"));
	}
}

