// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

// Needs to see how many players travel to the lobby level and, once a certain number comes in, travel to the Game Map

void ALobbyGameMode::PostLogin(APlayerController *NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // Access PlayerControllers that just joined -> GameState variable holds a GameStateBase that has an array of PLayerStates that have joined the Game
    int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
    if (NumberOfPlayers == 2)
    {
        // Travel using ServerTravel in the World Class
        UWorld *World = GetWorld();
        if (World)
        {
            bUseSeamlessTravel = true;                                    // To travel seamlessly
            World->ServerTravel(FString("/Game/Maps/BlasterMap?listen")); // We need to travel to this level and designate it to be an Open Lister Server (for Clients to connect to), use the ? to add additional options to address
        }
    }
}
