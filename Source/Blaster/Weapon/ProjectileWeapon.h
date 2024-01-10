
#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

/**
 *
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

private:
	//Variable for knowing the projectile class to spawn -> TSubclassOf to populate this variable with the class of a projectile or everything derived from it
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass; //TSubclassOf alows us to create a class object
};
