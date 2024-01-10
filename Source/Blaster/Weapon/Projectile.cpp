
#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Constructing Collision Box
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // World dynamic since it flyes through the air
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);								// Ignore everything except what's indicated
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);	// Block visibily
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block); // Block World Static Objects
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);					// Block SkeletalMesh from custom made Channel

	// Constructing MovementComponent and some of its parameters:
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // Bullet keeps rotation aligned to velocity (for if we add fall off for example, rotation follows that trajectory)
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	// As soon as projectile spawns, we can spawn the tracer emitter attached to bullet
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(), // If we wanted to attach it to any bone, we could do it here
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition // KeepWorldPosition spawns tracer at the collision box and keeps world position of the collision box
		);
	}

	// Binding OnHit to the OnComponentHit Delegate (in order to call a function on hit). We do it in Begin Play and not in the Constructor to avoid issues for binding too early:
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); // When CollisionBox hits, this binding triggers OnHit Callback function
	}
}

void AProjectile::OnHit(UPrimitiveComponent *HitComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
	// Cast BlasterCharacter to Other Actor so we can play an OnHit animation and get damaged
	ABlasterCharacter *BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		// Play HitReact Montage:
		BlasterCharacter->MulticastHit();
	}

	Destroy();
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	// Spawning impact emmiter particle system and playing impact sound. Both are replicated because are called via Destroyed()
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}
