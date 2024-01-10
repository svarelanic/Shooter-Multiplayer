
#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector &HitTarget)
{
    if (!HasAuthority()) // We do not want the projectile to be spawned outside of the Server since Weapon class is set to replicate
    {
        return;
    } // We will then replicate the projectile spawning separately

    Super::Fire(HitTarget);
    APawn *InstigatorPawn = Cast<APawn>(GetOwner()); // Gun owner set to instigator of APawn type for the SpawnParameters for projectile

    // SPAWNING PROJECTILE at the tip of the barrel:
    const USkeletalMeshSocket *MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash")); // getting socket projectile spawns from
    if (MuzzleFlashSocket)
    {
        // Getting socket's transform to knowe WHERE to spawn projectile
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        FVector ToTarget = HitTarget - SocketTransform.GetLocation(); // Vector from tip of the barrel to our HitTarget
        FRotator TargetRotation = ToTarget.Rotation();                // Rotation that represents that vector direction -> Used as the projectile spawn rotation

        if (ProjectileClass && InstigatorPawn) // if the class is set on the Editor
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner(); // return owner of our weapon which, when equiped, is the user
            SpawnParams.Instigator = InstigatorPawn;

            // SpawnActor is a function on the World, so we need to get a hold of it:
            UWorld *World = GetWorld();
            if (World)
            {
                // Requires an FActorSpawnParameters type, which  we defined oursevles above
                World->SpawnActor<AProjectile>(
                    ProjectileClass,
                    SocketTransform.GetLocation(),
                    TargetRotation,
                    SpawnParams);
            }
        }
    }
}
