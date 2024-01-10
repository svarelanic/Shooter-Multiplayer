// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 *
 */
UCLASS()
class BLASTER_API UOverheadWidget : public UUserWidget //This widget must be added to the Character
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget)) // Associtates the C++ variable with the WBP variable with the same name
	class UTextBlock *DisplayText; // Will show the Role on top of a User's head.

	void SetDisplayText(FString TextToDisplay);//to set the text for DisplayText text block
	
	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);//Figure oout the network Role to display

protected:
	virtual void NativeDestruct() override; //Called when transitioning from level and let's us remove widget from level
};
