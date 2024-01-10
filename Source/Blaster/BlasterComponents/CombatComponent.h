
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	friend class ABlasterCharacter; // makes the BlasterCharacter class have total access to all of CombatComponent's members
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon *WeaponToEquip);


protected:
	virtual void BeginPlay() override;

	// Called from CharacterClass, handles from both Server and Client side
	void SetAiming(bool bIsAiming);

	// RPC for Aiming -> Called in SetAiming
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	// Rep Notify to not orienting rotation to movement on controlling Client:
	UFUNCTION()
	void OnRep_EquippedWeapon();

	// Function to handle for when player is Firing, called from BlasterCharacter
	void FireButtonPressed(bool bPressed);

	//Function to handle firing
	void Fire();

	// Server RPC for firing -> Called from a Client and executed on the Server
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize &TraceHitTarget); // FVector_NetQuantize: Used just like FVector. Rounds our FVector to round numbers, sending data across network more efficiently
	// Multicast RPC for firing -> Called from server runs in all Clients, called from Client only runs in said Client
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize &TraceHitTarget);

	// Drawing a linetrace to know where we are shooting in relation to the Controller -> Pass the HitResult as an input parameter by reference
	void TraceUnderCrosshairs(FHitResult &TraceHitResult);

	// Setting crosshairs based on weapon used:
	void SetHUDCrosshairs(float DeltaTime); //Called on Tick Component

private:
	class ABlasterCharacter *Character;
	class ABlasterPlayerController* Controller;
	class ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon) // To equip in Client
	AWeapon *EquippedWeapon;

	// Aiming:
	UPROPERTY(Replicated) // Replicating from Server to Client, no info from Client to Server -> RPC from Client to Server
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	//HUD and Crosshairs:
	float CrosshairVelocityFactor; //The amount that contributes to the Crosshairs spread based on the Character's velocity
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	//Aiming zoom and FOV:
	//FOV when not aiming set to the camera's base FOV in BeginPlay
	float DefaultFOV;
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;
	//Current FOV, which we'll be setting in BeginPlay at the start and later changing on InterpFOV implementation
	float CurrentFOV;
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;
	//Function for the zoom when aiming called in Tick
	void InterpFOV(float DeltaTime);

	//Timer for automatic fire
	FTimerHandle FireTimer;

	//Function to start the FireTimer
	void StartFireTimer();
	//Callback function for when FireTimer has finished
	void FireTimerFinished();

	//boolean to see if gun CAN fire
	bool bCanFire = true;


public:
};
