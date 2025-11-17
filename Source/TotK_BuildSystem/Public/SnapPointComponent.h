#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SnapPointComponent.generated.h"

// Possible snap points for all moveable object blueprints
UENUM(BlueprintType)
enum class ESnapType : uint8 {
	Base UMETA(DisplayName = "Base"),
	BeamEnd UMETA(DisplayName = "Beam End"),
	BeamMiddle UMETA(DisplayName = "Beam Middle"),
	BoardTop UMETA(DisplayName = "Board Top"),
	BoardSide UMETA(DisplayName = "Board Side"),
	BoardFront UMETA(DisplayName = "Board Front"),
	FanBottom UMETA(DisplayName = "FanBottom"),
	WheelCenter UMETA(DisplayName = "Wheel Center"),
	WheelOuter UMETA(DisplayName = "Wheel Outer"),
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TOTK_BUILDSYSTEM_API USnapPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USnapPointComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
	ESnapType SnapType = ESnapType::Base;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
	TArray<ESnapType> CompatableSnapTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
	float SnapRadius = 25.f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Draw a debug sphere showing where the snapping point is located
	UFUNCTION()
	void DrawDebug();
};
