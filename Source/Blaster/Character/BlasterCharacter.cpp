
#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h" //To replicate -> DOREPLIFETIME
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "Blaster/Blaster.h"

// Constructor
ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Constructing CameraBoom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh()); // Attaching CameraBoom to our Mesh Component, not in Capsule Root since crouch will change capsule size
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true; // We can rotate the CameraBoom along with our Controller when adding Mouse input (needed for aiming)

	// Constructing FollowCamera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attaching to the CameraBoom's socket named SocketName
	FollowCamera->bUsePawnControlRotation = false;								// As it is attached to the CameraBoom

	// Setting for Rotation Animations -> Must set in Blueprint
	bUseControllerRotationYaw = false;						  // for our Character to not rotate alongside our Controller Rotation while unequipped -> Uncheck in Character Blueprint
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character orient towards its own movement direction -> Check in Character Blueprint

	// Adding OverheadWidget to Character:
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	// Constructing Combat Component
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); // Components do not need to be registered on GetLifetimeReplicatedProps, they only need to be SetIsReplicated to true

	// Setting bCanCrouch:
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// Making sure camera does not change when col|liding with other Characters:
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh); // Setting the collision type to be the Custom Collision channel we created
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// Setting Mesh to collide with weapon line trace channel -> Visibility Channel:
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// Setting default value for turning in Place -> Not Turning as default:
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	// Net update frequency for smooth network play
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	// Setting Turn Rate (when unnequiped, since when equipped we manage turning with YawOffset):
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Register variables to Replicate on Controlled Character on Clients:
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this; // Setting the Character variable on CombatComponent to be the BlasterCharacyer on PostIntialize
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage); // Plays the Montage
		// Choosing the Montage section:
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f; // Set it to zero since we just replicated movement
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage); // Plays the Montage
		// Choosing the Montage section:
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

AWeapon *ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr)
	{
		return nullptr;
	}
	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr)
	{
		return FVector(); // Return empty FVector
	}
	return Combat->HitTarget;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled()) // Simulated proxy is at the bottom of the Enum, everything above it means is locally controlled or on server
	{
		AimOffset(DeltaTime);
	} // Remember the ENetRole is defined in the UE library, in EngineTypes.h
	// For the Simulated Proxy, we will call it on OnRep_ReplicatedMovement instead of on the Tick
	else // We are simulated proxy
	{
		// We keep track of seconds between movement replication:
		TimeSinceLastMovementReplication += DeltaTime; // When it reaches a certain amount we call OnRep_ReplicatedMovement directly
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement(); // To call the SimProxyTurn() just in case
		}
		CalculateAO_Pitch(); // To calculate the pitch every frame and not every .25 seconds
	}

	HideCameraIfCharacterClose();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Binding Axis mapings with movement functions
	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);

	// Binding Jump
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);
	// Binding Equip Weapon
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	// Binding Crouch
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	// Binding start and end aim:
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	// Binding Fire
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0) // Controller is an inherited variable of the Character class of type AController
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);	 // Gettin the Yaw rotation of the Controller, not the Actor, so we turn with the Controller
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X)); // Matrix from rotator, and getting an FVector from GetUnitAxis that represents the direction of the Yaw Rotation, the Forward Vector
		AddMovementInput(Direction, Value);											 // Moves at the direction indicated by the Scale value from the editor
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y)); // Get the Cointroller's Right Vector
	AddMovementInput(Direction, Value);
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority()) // We are calling function from Server
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else // We are calling function from Client
		{
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch(); // If Crouching is successfull, bIsCrouch bool which is a public variable will be true and we can use it on the Anim BP
				  // Crouch function automatiacally resizes the Capsule Component for us
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime) // Called every Tick
{
	if (Combat && Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	// Calculation: - When standing still -> Move upper body up and down with Pitch and, with Yaw, we'll only move upper body 90 degrees and then move lower body to adjust. When runninf we'll only care about up and down Pitch
	//  Accessing Speed to see if we are moving or not:
	float Speed = CalculateSpeed();

	// Accessing if in air:
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // Standing still, not jumping
	{
		// Setting rotate root bone to true since we are only calling this if we are authority or autonomous proxy:
		bRotateRootBone = true;

		// YawOffset -> Delta between the base Yaw rotation and the Yaw current rotation
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		// Delta between current and starting aim rotation:
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(StartingAimRotation, CurrentAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		// If not turning in place already, we set the InterpAO_Yaw to be AO_Yaw:
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		bUseControllerRotationYaw = true; // To rotate the Controller with the Camera movement

		// Turn in place:
		TurnInPlace(DeltaTime);
	}

	// Storing base rotation when we just stopped, so the lasr rotation postion when moving:
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false; // don't need to rotate it while in the air or while running

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f; // Since we do not care about rotating from the hips up when running
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

float ABlasterCharacter::CalculateSpeed()
{
	//  Accessing Speed to see if we are moving or not:
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	// Setting Pitch:
	AO_Pitch = GetBaseAimRotation().Pitch;
	// Pitch and Yaw rotation are compressed to an unsigned value to send it across the network, which leads to postivice Pitch values when looking down from with the SimulatedProxy
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// Map Pitch from [270,360) range to [-90, 0) range
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch); // Takes AO_Pitch InRange and convertsit to the correct value in OutRange
	}

	UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
}

void ABlasterCharacter::SimProxiesTurn()
{
	// Handle turning for simulated proxies
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	// we do not want to rotate root bone in simulated proxy:
	bRotateRootBone = false;

	// If Speed is greater than zero, set our ETurningInPlace into Not Turning and return early:
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	// Play turn in place animations when we've rotated enough -> when turn in yaw reaches a defined threshold, we play the turn in place animation
	ProxyRotationLastFrame = ProxyRotation; // first rotation value
	ProxyRotation = GetActorRotation();		// final rotation value
	// Get the Delta between theese two rotations to see how much it rotated, normalized since we only care anput direction, only taking the Yaw component
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	// this delta we are calculating is not going to work as is, as it is designed to calculate a Delta from previous to current frame, but for the SimulatedProxy
	// we use NetUpdates, which are only every so often (not as frequent as frame). This is where replicated movement comes in palce:
	// Every time we replicate movement, there is a Rep Notify being called. We call the turning for sim proxies only when movement is getting updated, that's where OnRep_ReplicatedMovement comes in

	if (FMath::Abs(ProxyYaw) > TurnThreshold) // If the absolute value of the Delta Rotation is bigger than the defined threshold
	{
		// Play turn in place animations:
		if (ProxyYaw > TurnThreshold) // We are turning right (ProxyYaw is positive)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold) // We are turning left (ProxyYaw is negative)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else // In between -TurnTheshold and TurnThreshold -> we don't turn
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	// If we reach this line, we never enter insife the FMath::Abs(ProxyYaw) > TurnThreshold if check -> We don't turn, default behavior
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 60.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	// If we are turning in place, interpolate value for InterpAO_Yaw to zero:
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 5.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f) // If the absolute value of AO_Yaw is less than 15 degrees, que can set it to stop turning
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // Reset the Starting Aim Rotation
		}
	}
}

void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold) // Camera too close
	{
		GetMesh()->SetVisibility(false); // Hide mesh
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true; // Hide weapon mesh
		}
	}
	else // Camera not too close
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation() //_Implementation kw must be added on RPCs
{
	if (Combat) // If we do not have authority when pressing E, we send this RPC instead of EquipButtonPressed
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::OnRep_Health() //Called when Health gets replicated
{

}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon *Weapon)
{
	// Before setting the OverlappingWeapon = Weapon, we can hide the PickupWidget to hide it on the Server as well
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon; // Since OverlappingWeapon is OwnerOnly and SetOverlappingWeapon is called on the Server, if its LocallyControlled then it will not be replicated
	if (IsLocallyControlled())	// Checks if it is the Controlled Character only
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon *LastWeapon) // When adding an input param to a Rep notify, what gets passed in will be the last value before replication
{
	// Defining when is ShowPickupWidget from Weapon.h true:
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true); // Shown replicated on all Clients
	}

	// If we EndOverlap, OverlappingWeapon is set to null, which fails the if check above, but our last value for OverlappingWeapon still has the pointer to the Weapon we were overlapping, so we can hide the Widget for it

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false); // If overlapping weapon changes, we check its last value before change, and if not null we can hide the Widget
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}