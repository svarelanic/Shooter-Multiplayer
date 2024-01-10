// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "BlasterAnimInstance.generated.h"

/**
 *
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	// Overriding inherited functions
	virtual void NativeInitializeAnimation() override;			  // Called like BeginPlay
	virtual void NativeUpdateAnimation(float DeltaTime) override; // Called every frame like tike

private:
	// Variables to expose to animation BP:
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true")) // meta specifier allows the BP to acces the UPROPERTY
	class ABlasterCharacter *BlasterCharacter;												 // To store the Blaster Character using the AnimInstance

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating; // use it to drive animation if running or not

	// Animation BP needs to know when we have a Weapon Equipped:
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bWeaponEquipped;

	class AWeapon* EquippedWeapon; //For accessing Weapon's Socket for hand placement -> Getter on BlasterCharacter

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsCrouched;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	// Variables to handle Running Blend Space -> Leaning and Straifing:
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float YawOffset; // Driving our Straifing

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Lean; // Driving our Leaning
	
	//Variables needed to get Leaning:
	FRotator CharacterRotationLastFrame;
    FRotator CharacterRotation;
	//Variable for Rotation Interpolation of YawOffset to smooth animations -> Using RInterpTo to take shortest path possible in the animation when rotating
	FRotator DeltaRotation;

	//Variables for the Aim Offset (AO) in the Anim Graph:
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Yaw; 

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Pitch; 

	//Location for left hand placement with Inverse Kinematics -> Placement depends on Weapon Mesh so we add a Socket to weapon and place hand at Socket
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	//Variable for Turning in Place -> Set from the Getter on BlasterCharacter:
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	ETurningInPlace TurningInPlace;

	//Variable for rotating Gun into direction of the HitTarget -> Use it in BlasterAnimBP in editor to transform the bone after rotating root bone
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;

	// Variable for setting the IsLocallyControlled bool safely on BP:
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bLocallyControlled;
	
	//Variable for rotating root bone 
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bRotateRootBone;

};
