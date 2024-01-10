// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h" //For access to the Turning In Place enum
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
	// To Replicate:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override; // In implementation we register variables to be replicated
	virtual void PostInitializeComponents() override;													 // The sooner we can construct and get access to the CombatComponent is when executing this function

	// To play firing Montage
	void PlayFireMontage(bool bAiming);
	// Multicast to have all clients and Server using HitMontage
	UFUNCTION(NetMulticast, Unreliable) // Cosmetic, so not that important
	void MulticastHit();

	//Rep Notify from the ReplicatedMovement function from actor. We can use THIS instead of Tick when checking the Delta Yaw rotation of simulated proxies to play turning animations
	virtual void OnRep_ReplicatedMovement() override;

protected:
	virtual void BeginPlay() override;

	// Key Binding:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime); // For setting Yaw and Pitch offset in AimingOffset animation
	void CalculateAO_Pitch();
	void SimProxiesTurn();			 // Turn in place function for simulated proxies, as in simulated proxies rotate root bone turns with every NetUpdate, which is much slower than with every frame
	virtual void Jump() override;	 // To be able to stand up when crouch if Jump button is pressed
	void FireButtonPressed();
	void FireButtonReleased();

	// To play OnHit Montage
	void PlayHitReactMontage();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent *CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent *FollowCamera;

	// Adding OverheadWidget to Character:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true")) // Exposing private variable to BP
	class UWidgetComponent *OverheadWidget;

	// Stores Overlapped Weapon on Character for Replication:
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) // When it changes on the Server, changes on all Clients -> Must also override GetLifetimeReplicatedProps to Replicate
	class AWeapon *OverlappingWeapon;					 // Null until we set it on the Weapon Class -> We'll need a public setter for this

	UFUNCTION()										   // We will have OnRep_OverlappingWeapon be called on Client when OverlappinWeapon replicates to that Client. OnRep parameters can only be of the type of the replicated variable
	void OnRep_OverlappingWeapon(AWeapon *LastWeapon); // Rep Notifies do not get called on the Server, only when Variable Replicates from Server to Client (the Server will never get it called, so we must also handle showing the widget when we are the Server)

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent *Combat;

	// RPC to equipo Weapon from Client:
	UFUNCTION(Server, Reliable)		 // Reliable are guaranteed to be executed -> If the packet is droped, it will be resent
	void ServerEquipButtonPressed(); // Called on Client but executed on Server

	// Variables for setting Pitch and Yaw for AimOffset animation:
	float AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	// Variable for turning left or right depending on aiming:
	float InterpAO_Yaw;

	// For Turning in Place
	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	// Variables to handle the Firing Anim Montage and Getting Hit Montage
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage *FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage *HitReactMontage;

	void HideCameraIfCharacterClose(); // Called in tick

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 150.f;

	//Variable to determine if root boone rotates (in server or client) or not (in simulated proxy). We'll use it in the AnimBP in the editor 
	bool bRotateRootBone;
	//Number that defines how much we turn in the yaw before playing the turn in place animation in a simulated proxy
	float TurnThreshold = 0.5f;
	//Variables to keep in track how much our simulated proxies have rotated since the last frame
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	//Variable to store the Delta Rotation between frames to check how much the simulated proxy rotated
	float ProxyYaw;
	//Variable to keep track of the time since last movement replication. After a certain time we'll call the OnRep_ReplicatedMovement() to call SimProxyTurn() just to make sure it is being called regularly
	float TimeSinceLastMovementReplication;
	// Function to calculate Speed of characters movement -> When speed is not zero we do not turn in place with simulated proxies
	float CalculateSpeed();

	//Player Health
	UPROPERTY(EditAnywhere, Category = PlayerStats)
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = PlayerStats)
	float Health = 100.f; //Since its replicating, remember registering it to the GetLifetimeReplicatedProps using DOREPLIFETIME
	//Replicate Health using a Rep Notify. Every time the Health value is replicated down to the client, we can react to that, like updating character's HUD
	UFUNCTION()
	void OnRep_Health();

	// Second public area reserved for Getters and Setters for our member variables
public:
	// Setter for OverlappingWeapon to use on Character for Replication -> Will also handle showing PickupWidget on Server:
	void SetOverlappingWeapon(AWeapon *Weapon); // Only called on SERVER (since its called on OnSphereOverlap, only called on Server)
	// Getter that returns True or false based on wether or not a Weapon is equipped
	bool IsWeaponEquipped();
	// Getter that returns True or false based on wether or not we are Aiming -> Used to change aiming pose
	bool IsAiming();
	// Getters for setting AO_Yaw and AO_Pitch on BlasterAnimInstance:
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	// Getter for Equipped Weapon
	AWeapon *GetEquippedWeapon();
	// Getter for Turning In Place:
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	// Getter for HitTarget from the CombatComponent
	FVector GetHitTarget() const;
	// Getter for the FollowCamera for aiming zoom FOV:
	FORCEINLINE UCameraComponent *GetFollowCamera() const { return FollowCamera; }
	// Getter for bRotateRootBone variable for rotating in AnimInstance
	FORCEINLINE bool ShouldRotateRootBone() const {return bRotateRootBone; }
};
