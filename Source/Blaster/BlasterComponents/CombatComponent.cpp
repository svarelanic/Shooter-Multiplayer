
#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h" //For DOREPLIFETIME
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed; // Setting the Walk Speed we defined in the Movement Component

		// Setting camera's FOV -> By getting the FollowCamera in the BlasterCharacter class:
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Getting the HitTarget -> To align the weapon rotation to the Hit Target in BlasterAnimInstance
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint; // Our blaster anim instance will need to use this to correctly rotate the rifle muzzle -> Make a getter on BlasterCharacter for anim instance to access it

		SetHUDCrosshairs(DeltaTime); // Drawing the HUD Crosshairs
		InterpFOV(DeltaTime);		 // Zooming when aiming
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	// Needs to set package in BlaserHUD:
	if (Character == nullptr || Character->Controller == nullptr)
	{
		return;
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller; // If Controller is nullptr, we get the Controller from the BlasterCharacter, if not we set it as the Controller so everytime we tick we don't do the Cast
	if (Controller)
	{
		// Set the HUD:
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD; // Set the HUD the first time and all other times, since its called on Tick, we set it to HUD which is really cheap
		if (HUD)
		{
			// Set HUD package from BlasterHUD.h SetHUDPackage function:
			if (EquippedWeapon)
			{
				// Set the Crosshairs textures from the Weapon class into the HUDPackage from the BlasterHUD class
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else // We don't have a weapon equipped, so we don't want crosshairs
			{
				// Set the Crosshairs textures from the Weapon class into the HUDPackage from the BlasterHUD class
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}

			// Calculate Crosshair Spread -> create a value between 0 (not moving) and 1 (MaxWalkSpeed for normal movement) -> We must change the Speed value from for example [0,600] to [0,1]
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed); // Walking range
			FVector2D VelocityMultiplierRange(0.f, 1.f);									//[0,1] range
			FVector Velocity(Character->GetVelocity());
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size()); // Maps the Velocity.Size value to the output [0,1] range -> takes two ranges and changes from one to the other
																																   // Returns the Velocity value mpped from WalkSpeedRange to VelocityMultiplierRange
			if (Character->GetCharacterMovement()->IsFalling())
			{
				// Spreading crosshairs while in the air -> Interpolate using DeltaTime to spread more slowly, and spread even more than from [0,1]
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f); // first float is the Target and second one is the InterpSpeed
			}
			else // When we hit the ground
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f); // We interp to a target of zero and much faster as soon as we hit the ground
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f); // Interp to shrinking Crosshairs when aiming
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f); // Interp to zero when not aiming
			}

			// Interpolating the shooting spread back to zero every time:
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 20.f);

			// Sets it on the BlasterHUD Struct:
			HUDPackage.CrosshairSpread = 0.5f +
										 CrosshairVelocityFactor +
										 CrosshairInAirFactor -
										 CrosshairAimFactor +
										 CrosshairShootingFactor; // Each time the weapon is fired this will be increased -> increase it on FireButtonPressed

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed()); // Interp from CurrentFOV to the ZoomedFOV from Weapon.h, at a ZoomInterSpeed
	}
	else // Not aiming, interp back to the  DefaultFOV
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed); // UnZooming back to normal happens at the same speed regardless of Weapon used, so we used the ZoomInterpSpeed defined on the CombatComponent
	}

	// Setting the FOV on the Camera:
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming); // If we are on the Server, it will run on the Server, if we are on the Client it will be executed on Server
	// Set walk speed for server:
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming; // Set the replicated variable
	// Set walk speed for client:
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult &TraceHitResult) // Out parameter
{
	// To trace from center of the screen we need the Viewport size
	FVector2D ViewportSize; // Value given as an out parameter
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// Center of Viewport:
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); // Divided by 2 to get the center (where the Crosshairs will be)
	// Getting the ScreenSpace coordinates from above in WorldSpace:
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld( // Returns true if deprojection was succesful
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition, // out parameter
		CrosshairWorldDirection // out parameter
	);

	if (bScreenToWorld) // Succesfully got World Coordinates of the Screen Center, we now perform linetrace
	{
		FVector Start = CrosshairWorldPosition;

		//Making LineTrace start after the Character and not after the Camera, so that no collision errors occur
		if(Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f); //Push the start location forward in the Crosshair World Direction 
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; // TRACE_LENGTH is 80000 (as for the define), which extends the CrosshairsWorldDirection (which is a one unit vector)

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult, // out parameter
			Start,
			End,
			ECollisionChannel::ECC_Visibility // Collision channel, hits whatever it sees
		);

		//InteractWithCrosshairsInterface functionallity -> Crosshairs changes colors 
		if(TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>()) //If we Hit an actor with our Line Trace AND said actor implements the Interface (for Crosshair to change colors)
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed) // always called locally on Client or Server
{
	bFireButtonPressed = bPressed; // We know wether the button is pressed or released

	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{

	if(bCanFire)
	{
		bCanFire = false; //Set it as true only after our FireTimer has finished

		ServerFire(HitTarget); // Called from the Client on the Server. Pass in HitResult.ImpactPoint as parameter, so MulticastFire also needs a HitResult as parameter

		//Start a timer and, at the end of that timer, call ServerFire again if button is still pressed

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 0.75f;
		}
		
		StartFireTimer(); //Starts the timer, with a callback that determines if button still pressed, then keep firing -> As long as weapon is automatic
	}
}

void UCombatComponent::StartFireTimer()
{
	if(EquippedWeapon == nullptr || Character == nullptr)
	{
		return;
	}

	//Start the Timer:
	Character->GetWorldTimerManager().SetTimer(
		FireTimer, //The timer handle
		this, //The user object
		&UCombatComponent::FireTimerFinished, //Function callback of the timer
		EquippedWeapon->FireDelay //Timer delay
	);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true; //This way, we can only fire again once the .15 seconds have passed
	//Executed at the end of our timer -> Checks if we have fire button pressed, if true we want to fire the weapon
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize &TraceHitTarget)
{
	MulticastFire(TraceHitTarget); // When Client fires, calls the Server RPC that results in Server executing Multicast RPC, resulting in fire effect on all machines
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize &TraceHitTarget)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}
	if (Character)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::EquipWeapon(class AWeapon *WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr)
		return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket *HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character); // SetOwner already Replicates, so no need to replicate it on OnRep. We could override OnRep_Owner if we needed to as well

	// On weapon equipped, the Character's forward vector's direction will always be the same as where the camera is facing
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}