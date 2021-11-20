// Fill out your copyright notice in the Description page of Project Settings.


#include "PickHint.h"

bool UPickHint::Initialize()
{
	Super::Initialize();
	if (PickGunHint != nullptr)
	{
		this->SetVisibility(ESlateVisibility::Hidden);
		this->PickGunHint->SetText(FText::FromString("Press F to switch to"));
	}
	return true;
}

void UPickHint::OnNextWeaponChange(FString Input)
{
	if (Input != "")
	{
		FText TempText = FText::FromString("Press F to switch to" + Input);
		if (PickGunHint != nullptr)
		{
			this->SetVisibility(ESlateVisibility::Visible);
			this->PickGunHint->SetText(TempText);
		}
	}
	else
	{
		this->SetVisibility(ESlateVisibility::Hidden);
	}
}
