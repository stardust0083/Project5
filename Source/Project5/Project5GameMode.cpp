// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project5GameMode.h"
#include "Project5HUD.h"
#include "Project5Character.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

AProject5GameMode::AProject5GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AProject5HUD::StaticClass();
	
}
void AProject5GameMode::StartPlay()
{
	Super::StartPlay();
}

