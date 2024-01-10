
#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"

void ABlasterHUD::BeginPlay()
{
    Super::BeginPlay();

    AddCharacterOverlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
    APlayerController* PlayerController = GetOwningPlayerController(); //Get Player controller to add the widget to the ViewPort
    if(PlayerController && CharacterOverlayClass) 
    {
        //Create the Widget:
        CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
        CharacterOverlay->AddToViewport(); //Add the Widget to the Controller's Viewport
    }
}

void ABlasterHUD::DrawHUD()
{
    Super::DrawHUD();

    // Crosshairs: on Weapon Class as we may want to change it depending on weapon
    FVector2D ViewportSize; // Out parameter top get the Viewport size to know where to draw the crosshairs on screen
    if (GEngine)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        const FVector2D ViewPortCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); // Initialized under the ()

        float SpreadScale = CrosshairSpreadMax * HUDPackage.CrosshairSpread; // The CrosshairSpread from the Struct will be changing that spread dinamically, being scaled by the CrosshairSpreadMac variable (which we can edit until we're happy with)

        // Drawinf the textures
        if (HUDPackage.CrosshairsCenter)
        {
            FVector2D Spread(0.f, 0.f); // No spread for the center point of the Crosshairs
            DrawCrosshair(HUDPackage.CrosshairsCenter, ViewPortCenter, Spread, HUDPackage.CrosshairsColor);
        }
        if (HUDPackage.CrosshairsLeft)
        {
            FVector2D Spread(-SpreadScale, 0.f); // Spread for left crosshairs -> Move it left (negative) in the X axis of the FVector2D
            DrawCrosshair(HUDPackage.CrosshairsLeft, ViewPortCenter, Spread, HUDPackage.CrosshairsColor);
        }
        if (HUDPackage.CrosshairsRight)
        {
            FVector2D Spread(SpreadScale, 0.f); // Opposite as left
            DrawCrosshair(HUDPackage.CrosshairsRight, ViewPortCenter, Spread, HUDPackage.CrosshairsColor);
        }
        if (HUDPackage.CrosshairsTop)
        {
            FVector2D Spread(0.f, -SpreadScale); // Moving up in Y axis -> Moving up is negative Y direction in UV coordinates
            DrawCrosshair(HUDPackage.CrosshairsTop, ViewPortCenter, Spread, HUDPackage.CrosshairsColor);
        }
        if (HUDPackage.CrosshairsBottom)
        {
            FVector2D Spread(0.f, SpreadScale); // Moving down in the positive Y direction
            DrawCrosshair(HUDPackage.CrosshairsBottom, ViewPortCenter, Spread, HUDPackage.CrosshairsColor);
        }
    }
}

void ABlasterHUD::DrawCrosshair(UTexture2D *Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
    const float TextureWidth = Texture->GetSizeX();
    const float TextureHeight = Texture->GetSizeY();

    const FVector2D TextureDrawPoint(ViewportCenter.X - (TextureWidth / 2.f) + Spread.X, ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y); // Center of the screen, move to left by TextureWidth/2 and moved up by TextureHeight/2

    DrawTexture(
        Texture,
        TextureDrawPoint.X,
        TextureDrawPoint.Y,
        TextureWidth,
        TextureHeight,
        0.f,
        0.f,
        1.f,
        1.f,
        CrosshairColor);
}
