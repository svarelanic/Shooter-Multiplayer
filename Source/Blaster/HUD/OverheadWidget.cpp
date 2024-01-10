// Fill out your copyright notice in the Description page of Project Settings.

#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
    if (DisplayText)
    {
        DisplayText->SetText(FText::FromString(TextToDisplay));
    }
}

void UOverheadWidget::ShowPlayerNetRole(APawn *InPawn)
{
    // Showing LocalRole as the Server will get every character as Authority, showing RemoteRole will get the controller as AutonomousProxy and the rest as SimulatedProxy
    // Showing LocalRole as the Client will will get the controller as AutonomousProxy and the rest as SimulatedProxy, showing RemoteRole will get every character as Authority

    ENetRole LocalRole = InPawn->GetLocalRole();

    // Calling SetDisplayText to show the Role for each case:
    FString Role;
    switch (LocalRole)
    {
    case ENetRole::ROLE_Authority:
        Role = FString("Authority");
        break;
    case ENetRole::ROLE_AutonomousProxy:
        Role = FString("AutonomousProxy");
        break;
    case ENetRole::ROLE_SimulatedProxy:
        Role = FString("SimulatedProxy");
        break;
    case ENetRole::ROLE_None:
        Role = FString("None");
        break;
    default:
        break;
    }

    FString PlayerName;
    APlayerState *PlayerState = InPawn->GetPlayerState();
    if (PlayerState)
    {
        PlayerName = PlayerState->GetPlayerName();
    }
    
    FString LocalRoleString = FString::Printf(TEXT("Local Role: %s || Name: %s"), *Role, *PlayerName);
    SetDisplayText(LocalRoleString);
}

void UOverheadWidget::NativeDestruct()
{
    RemoveFromParent(); // Take widget away from Viewport
    Super::NativeDestruct();
}
