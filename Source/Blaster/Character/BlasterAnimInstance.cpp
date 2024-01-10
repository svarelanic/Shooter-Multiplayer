
#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner()); // TryGetPawnOwner() returns Pawn Type
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    if (BlasterCharacter == nullptr)
    {
        BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
    }

    if (BlasterCharacter == nullptr)
    {
        return;
    }

    // Accessing Speed:
    FVector Velocity = BlasterCharacter->GetVelocity();
    Velocity.Z = 0.f;        // Only want lateral speed, don't care about the Z component
    Speed = Velocity.Size(); // Z component zeroed out, so Size returns the Speed

    // Setting Variables section:
    //  Setting bIsInAir:
    bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling(); // If IsFalling, we now we are in Air and play proper animations
    // Setting bIsAccelerating -> If we are adding input to move:
    bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
    // The ? operator takes the first expression (where we get the Size > 0.f), evaluates it and the "true" if true and the "false" of false. Like using if and else
    // Setting bWeaponEquipped:
    bWeaponEquipped = BlasterCharacter->IsWeaponEquipped(); // We must replicate EquippedWeapon for this to work on Client
    // Setting EquippedWeapon:
    EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
    // Setting bIsCrouched:
    bIsCrouched = BlasterCharacter->bIsCrouched;
    // Setting bAiming
    bAiming = BlasterCharacter->IsAiming();
    // Setting TurningInPlace
    TurningInPlace = BlasterCharacter->GetTurningInPlace();
    // Setting bRotateRootBone
    bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();

    // For this, we are using replicated values (like all the Get Rotations), so there is no need to replicate
    //  Setting YawOffset for Strafing:
    FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();                                 // 0 to 180 to the right and 0 to -180 to the left -> It is a GLOBAL base rotation, not local to the Character
    FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity()); // 0 to 180 to the right and 0 to -180 to the left -> It is a GOBAL base rotation
    // Rotation Interpolation so YawOffset changes smoothly if changes rapidly when strafing left to strafing right -> To not transition through all intermediate animations between -180 and 180 in a flash (takes shortest path in that rotation, from -180 directly to 180):
    FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
    DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f); // Smoothly interpolating rotation
    YawOffset = DeltaRotation.Yaw;

    // Setting Lean for Leaning -> Lean comes from Delta Yaw Rotation between character rotation itself and its own rotation from previous frame:
    CharacterRotationLastFrame = CharacterRotation;                                                                   // Set the last frame to character rotation AND THEN set Character Rotation
    CharacterRotation = BlasterCharacter->GetActorRotation();                                                         // Set the current Rotation
    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame); // Very small value -> Make it bigger and make sure its scaled by delta time for FPS independency:
    // Delta is a very small value -> Make it bigger and make sure its scaled by delta time for FPS independency:
    const float Target = Delta.Yaw / DeltaTime;
    const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f); // Interpolate to smoothly transition between the Delta value (very small) and the scaled, frame independant Target value
    Lean = FMath::Clamp(Interp, -90.f, 90.f);                            // Set Lean to be Interp value between 90 degrees and -90 degrees (to make up for quick mouse movement)

    // Setting Yaw and Pitch for AimOffset animation:
    AO_Yaw = BlasterCharacter->GetAO_Yaw();
    AO_Pitch = BlasterCharacter->GetAO_Pitch();

    // Setting Left Hand Placement Transform on weapon Socket
    if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
    {
        // Get socket transform from EquippedWeapon socket:
        LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
        // Change from WorldSpace to BoneSpace on the Skeleton of our Mesh:
        // Out parameters of TransformToBoneSpace:
        FVector OutPosition;
        FRotator OutRotation;
        BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("Hand_R"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation); // Name of bone on our Skeleton we want our Weapon SocketTransform to transform into the space of->Have the socket of the weapon relative to our right hand
        // After calling this, the Out parameter will have the position and rotation of the left hand socket transformed to hand_r bonespace
        // With the out parameters, we can set the rotation and location of LeftHandTransform:
        LeftHandTransform.SetLocation(OutPosition);
        LeftHandTransform.SetRotation(FQuat(OutRotation));

        if (BlasterCharacter->IsLocallyControlled())
        {
            bLocallyControlled = true;
            // Fixing gun rotation for the muzzle to point at the HitTarget at all times -> By moving the Right Hand:
            FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World); // GetSocketTransform can also take in Bone names
            // Returning an FRotator corresponding to the rotation of the Vector from the start to the target:
            FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
            RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f); // Interp so our hand adjust smoothly when aiming at something close to the character
        }
    }
}
