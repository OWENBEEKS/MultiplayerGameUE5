// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if(NumberOfPlayers >= 2)
	{
		UWorld* world = GetWorld();
		if (world)
		{
			bUseSeamlessTravel = true; // Enable seamless travel
			world->ServerTravel(FString("/Game/Maps/ShooterMap?listen"));
		}
	}
	else
	{
		// Optionally, you can send a message to the player that they are waiting for more players
		NewPlayer->ClientMessage(TEXT("Waiting for more players to join..."));
	}
}
