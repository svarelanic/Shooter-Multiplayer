
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage //Holds all crosshairs we're supposed to draw each frame
{
	GENERATED_BODY()

public:
	//Adding texture2d pointers for the crosshairs 
	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	float CrosshairSpread; //Used when drawing texture that tells us how much it expands when shooting. Set it on Combat Component
	FLinearColor CrosshairsColor;
};

UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public: 
	virtual void DrawHUD() override; //Called every frame, we can determine what gets draw to the screen here

	//TSubclassOf that we can select in the BlasterHUD BP that will allow us to have the UClass we need when we create the Widget
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass; //We can then create the CharacterOverlay Widget Node once we set it on the BP, so we can showcase it from the Controllers view

	class UCharacterOverlay* CharacterOverlay; //To access the Character Overlay -> Will be used by the CharacterController

protected:
	virtual void BeginPlay() override;
	void AddCharacterOverlay();

private:
	FHUDPackage HUDPackage; //Variable that contains all the texture2d objects on the sruct

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor); //Does the math to center the texture (since if we place it on the center of the viewport as is, it will center from the top right conrner)
																						// When we call DrawCrosshair, we'llpass in an FVector2D with the right amount to spread in X and Y, we're spreading the same amount (CrosshairSpread) in X and Y
	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) {HUDPackage = Package; }; //Public setter for HUDPackage, Crosshairs is set from the Combat Component
};
