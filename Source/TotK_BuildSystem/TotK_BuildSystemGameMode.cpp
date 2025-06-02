// Copyright Epic Games, Inc. All Rights Reserved.

#include "TotK_BuildSystemGameMode.h"
#include "TotK_BuildSystemCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATotK_BuildSystemGameMode::ATotK_BuildSystemGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
