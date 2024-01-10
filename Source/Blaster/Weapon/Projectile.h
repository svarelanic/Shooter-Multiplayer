
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override; //Overriding it calls it also on Clients. Called automatically when actor is explicitly Destroyed (we use Destroy() OnHit).

protected:
	virtual void BeginPlay() override;

	//Function to handle Hit events on hit, make it virtual in case we make child classes of projectile. Hit is an outparameter containing the HitResult info
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(EditAnywhere)
	class UBoxComponent *CollisionBox;

	// Movement component to handle movement -> does replication
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent *ProjectileMovementComponent;

	// Variable for storing a Particle System, setting it from BP
	UPROPERTY(EditAnywhere)
	class UParticleSystem *Tracer;
	// Once we spawn this tracer, we can store it on a UParticleSystemComponent variable:
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

public:
};
