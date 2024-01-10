
#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	// Constructing static mesh component:
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true); // Allows physics simulation to generate the Hit Event as soon as we hit something. Named 'Simulation Generates Hit Events' in Blueprint

	ShellEjectionImpulse = 5.f; // Gives impulse to the direction
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit); // Binding the OnHit function as a callback on a HitEvent

	// Adding a random impulse to the casing:
	FVector CasingMeshImpusleVector = GetActorForwardVector();
	CasingMeshImpusleVector.Z = FMath::RandRange(-0.3f, 0.3f);
	CasingMesh->AddImpulse(CasingMeshImpusleVector * ShellEjectionImpulse);

	SetLifeSpan(3.f);
}

void ACasing::OnHit(UPrimitiveComponent *HitComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}
	// Destroy();
}
