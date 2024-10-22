// Copyright Epic Games, Inc. All Rights Reserved.

#include "MHProjectGameMode.h"
#include "MHProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMHProjectGameMode::AMHProjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
