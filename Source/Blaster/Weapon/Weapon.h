
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	// Defining the weapon states
	EWS_Initial UMETA(DisplayName = "Initial State"), // Weapon will be set on the first enum state by default
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMax") // By checking its numerical value, we can check how many enum constants are in this enum

};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
	void ShowPickupWidget(bool bShowWidget);
	// Virtual fire so we can override it in different Weapon types (projectile weapon for example)
	virtual void Fire(const FVector &HitTarget); // HitTarget added to know what it hits, passed as const reference for efficiency (so we don't have to copy the HitTarget into the function)

	// Textures for the weapon crosshairs:
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D *CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D *CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D *CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D *CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D *CrosshairsBottom;

	//Delay in between automatic fire
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	//boolean to see if our gun is autmatic
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

protected:
	virtual void BeginPlay() override;

	UFUNCTION() // Must be a UFUNCTION as we are binding it to AreaSphere with a Delegate: OnComponentBeginOverlap
	virtual void OnSphereOverlap(UPrimitiveComponent *OverlappedComponent,
								 AActor *OtherActor,
								 UPrimitiveComponent *OtherComp,
								 int32 OtherBodyIndex,
								 bool bFromSweep,
								 const FHitResult &SweepResult); // Overlap functions must have these parameters

	UFUNCTION()
	virtual void OnSphereOverlapEnd(UPrimitiveComponent *OverlappedComp,
									AActor *OtherActor,
									UPrimitiveComponent *OtherComp,
									int32 OtherBodyIndex);

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent *WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent *AreaSphere; // Used to detect overlaps with characters using OnSphereOverlap, once overlap we can pick up Weapon.

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent *PickupWidget;

	// Animation asset for firing animations
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset *FireAnimation;

	// What class to spawn when ejecting a Casing:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	// Variables for zoomed FOV while aiming:
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent *GetAreaSphere() const { return AreaSphere; }
	// Getter for Weapon Mesh for Left Hand Placement:
	FORCEINLINE USkeletalMeshComponent *GetWeaponMesh() const { return WeaponMesh; }
	//Getters for Zoom FOV Variables:
	FORCEINLINE float GetZoomedFOV() const {return ZoomedFOV;}
	FORCEINLINE float GetZoomInterpSpeed() const {return ZoomInterpSpeed;}
};
