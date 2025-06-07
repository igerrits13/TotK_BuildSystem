// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Grabber.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TOTK_BUILDSYSTEM_API UGrabber : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrabber();

	UFUNCTION(BlueprintCallable)
	void Grab();

	UFUNCTION(BlueprintCallable)
	void Release();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MaxGrabDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float GrabRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MinHoldDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Settings")
	float MaxHoldDistance = 250.f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	// Physics handle for moving objects
	UPhysicsHandleComponent* PhysicsHandle;

	// Vector for offsetting the height of held objects caused by third-person camera
	FVector CameraOffsetVector;

	bool GetGrabbableInReach(FHitResult& OutHitResult) const;
};
