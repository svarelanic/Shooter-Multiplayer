

#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h" //To show Weapon Pickup Widget only to Blaster Character on Overlap
#include "Net/UnrealNetwork.h"					//For DOREPLIFETIME
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false; // We do not need it to Tick for now

	// For the Server to be in charge of all Weapon objects, we have to send it to REPLICATE:
	bReplicates = true;

	// Constructing weapon Mesh Component:
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WEaponMesh"));
	SetRootComponent(WeaponMesh);
	// Setting collision to block for all channels for weapon for when DROPPING WEAPON
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore); // Ignoring the Pawn once dropped:
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);										// Weapon Starts with no Collision, only getting it when dropped

	// Constructing Weapon Shpere Component:
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	// In Multiplayer game, area sphere detecting overlap events for Weapon pick up should be done in the Server (for its importance)
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Disable Collision on the constructor and enabling it in BeginPlay only on the SERVER

	// Constructing Pickup Widget Component:
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Activating Collision and Overlap Events for the AreaSphere only for Server
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap); // Collsion response set to the Channel we want to overlap with -> The PawnCollision type
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);						 // Binding OnSphereOverlap to AreaSphere only on SERVER
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereOverlapEnd);
	}

	// Making Pickup Widget invisible by default (only visible on Overlap with BlasterCharacter)
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	ABlasterCharacter *BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) // If Cast succeeded, BlasterCharacter has Overlapped with the AreaSphere
	{
		BlasterCharacter->SetOverlappingWeapon(this); // Making this weapon the Overlapping Weapon
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;

	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}

void AWeapon::OnSphereOverlapEnd(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter *BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) // If Cast succeeded, BlasterCharacter has Overlapped with the AreaSphere
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr); // Make it null, but the Rep Notify will save the last value of the Replicated PickupWidget to access and show again
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector &HitTarget)
{
	if (FireAnimation)
	{
		// We can play an animation using our skeletal mesh
		WeaponMesh->PlayAnimation(FireAnimation, false); // False means we don't want to loop the Animation
	}

	// Spawning Casing when firing weapon
	if (CasingClass)
	{
		const USkeletalMeshSocket *AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject")); // getting socket Casing spawns from
		if (AmmoEjectSocket)
		{
			// Getting socket's transform to knowe WHERE to spawn projectile
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			// SpawnActor is a function on the World, so we need to get a hold of it:
			UWorld *World = GetWorld();
			if (World)
			{
				// Requires an FActorSpawnParameters type, which  we defined oursevles above
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}
}
