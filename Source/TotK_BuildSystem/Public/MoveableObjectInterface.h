// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MoveableObjectInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UMoveableObjectInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for the grabber to interact with moveable objects
 */
class TOTK_BUILDSYSTEM_API IMoveableObjectInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Moveable Object")
	void OnGrab();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Moveable Object")
	void OnRelease();
};
