// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "MPShooter/Weapon/Weapon.h"
#include "MPShooter/ShooterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

AMultiplayerCharacter::AMultiplayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->SetWalkableFloorAngle(46.f);

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}


void AMultiplayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AMultiplayerCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void AMultiplayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (Controller)
	{
		UE_LOG(LogTemp, Display, TEXT("%s is possessed by %s"), *GetName(), *Controller->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s is not possessed"), *GetName());
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		UE_LOG(LogTemp, Display, TEXT("Player Controller is %s"), *PlayerController->GetName());
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
				UE_LOG(LogTemp, Display, TEXT("Input Mapping Context added"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("InputMapping is null!"));
			}
		}
	}
}

void AMultiplayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);
}

void AMultiplayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void AMultiplayerCharacter::Move(const FInputActionInstance& Instance)
{
	FVector2D MovementDirection = Instance.GetValue().Get<FVector2D>();
	const FRotator Rotation(0.f, Controller->GetControlRotation().Yaw, Controller->GetControlRotation().Roll);
	const FVector RightDirection(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y));
	const FVector ForwardDirection(FRotationMatrix(Rotation).GetUnitAxis(EAxis::X));
	AddMovementInput(RightDirection, MovementDirection.X);
	AddMovementInput(ForwardDirection, MovementDirection.Y);
}

void AMultiplayerCharacter::Look(const FInputActionInstance& Instance)
{
	FVector2D LookDirection = Instance.GetValue().Get<FVector2D>();
	AddControllerYawInput(LookDirection.X);
	AddControllerPitchInput(LookDirection.Y);
}

void AMultiplayerCharacter::Jump()
{
	Super::Jump();
	UE_LOG(LogTemp, Warning, TEXT("Jump action triggered."));
}

void AMultiplayerCharacter::EquipButtonPressed(const FInputActionInstance& Instance)
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void AMultiplayerCharacter::CrouchButtonPressed(const FInputActionInstance& Instance)
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AMultiplayerCharacter::AimButtonPressedFunc(const FInputActionInstance& Instance)
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AMultiplayerCharacter::AimButtonReleasedFunc(const FInputActionInstance& Instance)
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void AMultiplayerCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // Standing still and not jumping
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = false;
	}
	if(Speed> 0.f || bIsInAir) // Running or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// Map pitch from [270,360) to [-90,0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AMultiplayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			UE_LOG(LogTemp, Display, TEXT("Binding MoveAction"));
			Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMultiplayerCharacter::Move);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MoveAction is null!"));
		}
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMultiplayerCharacter::Look);
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AMultiplayerCharacter::Jump);
		Input->BindAction(EquipButtonAction, ETriggerEvent::Started, this, &AMultiplayerCharacter::EquipButtonPressed);
		Input->BindAction(CrouchButtonAction, ETriggerEvent::Started, this, &AMultiplayerCharacter::CrouchButtonPressed);
		Input->BindAction(AimButtonPressed, ETriggerEvent::Started, this, &AMultiplayerCharacter::AimButtonPressedFunc);
		Input->BindAction(AimButtonReleased, ETriggerEvent::Completed, this, &AMultiplayerCharacter::AimButtonReleasedFunc);
	}

}

void AMultiplayerCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void AMultiplayerCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool AMultiplayerCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AMultiplayerCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

void AMultiplayerCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}