// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiCharAnimInstance.h"
#include "MultiplayerCharacter.h"

void UMultiCharAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	PlayerCharacter = Cast<AMultiplayerCharacter>(TryGetPawnOwner());
}

void UMultiCharAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(PlayerCharacter == nullptr)
	{
		PlayerCharacter = Cast<AMultiplayerCharacter>(TryGetPawnOwner());
	}
	if (PlayerCharacter == nullptr) return;

	FVector Velocity = PlayerCharacter->GetVelocity();
	Velocity.Z = 0.f; // Ignore Z velocity for movement calculations
	Speed = Velocity.Size();

	bIsInAir = PlayerCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
}