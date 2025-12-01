// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputMappingContext.h"
#include "MPShooter/ShooterTypes/TurningInPlace.h"
#include "MPShooter/Interfaces/InteractWithCrosshairsInterface.h"

#include "MultiplayerCharacter.generated.h"

UCLASS()
class MPSHOOTER_API AMultiplayerCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	AMultiplayerCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();

	virtual void OnRep_ReplicatedMovement() override;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();
	
	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* InputMappingContext;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* EquipButtonAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* CrouchButtonAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* AimButtonPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* AimButtonReleased;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* FireButtonPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* FireButtonReleased;

protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionInstance& Instance);
	void Look(const FInputActionInstance& Instance);
	virtual void Jump() override;
	void EquipButtonPressed(const FInputActionInstance& Instance);
	void CrouchButtonPressed(const FInputActionInstance& Instance);
	void AimButtonPressedFunc(const FInputActionInstance& Instance);
	void AimButtonReleasedFunc(const FInputActionInstance& Instance);
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	void FireButtonPressedFunc();
	void FireButtonReleasedFunc();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere, Category = Camera)
	float CameraThresholdDistance = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
};