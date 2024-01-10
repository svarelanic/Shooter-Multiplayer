// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget)) //Binds it to the ProgressBar in our CharacterOverlayWidget in the Editor
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget)) 
	class UTextBlock* HealthText;


};
