// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project25DGameMode.h"
#include "Project25DCharacter.h"
#include "UObject/ConstructorHelpers.h"

AProject25DGameMode::AProject25DGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
