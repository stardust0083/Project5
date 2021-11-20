// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "PickHint.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT5_API UPickHint : public UUserWidget
{
	GENERATED_BODY()
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* PickGunHint;

	virtual bool Initialize() override;
public:
	UFUNCTION()
	void OnNextWeaponChange(FString Input);
};
