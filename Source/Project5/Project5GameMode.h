// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Project5GameMode.generated.h"

UCLASS(minimalapi)
class AProject5GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AProject5GameMode();
	virtual void StartPlay() override;

};
