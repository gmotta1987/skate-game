#include "Characters/SkateCharacter.h"

#include "Actors/SkateboardActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

ASkateCharacter::ASkateCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 340.0f;
    CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 10.0f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 12.0f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ASkateCharacter::BeginPlay()
{
    Super::BeginPlay();

    GetCharacterMovement()->DisableMovement();

    // A blank Unreal level may start the pawn at world origin. Raise the
    // prototype rider enough to let the board fall naturally onto the arena.
    FVector SafeStart = GetActorLocation();
    SafeStart.Z = FMath::Max(SafeStart.Z, RiderHeight + 80.0f);
    SetActorLocation(SafeStart);

    SpawnSkateboard();
}

void ASkateCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FollowSkateboard(DeltaSeconds);
    UpdateDebugOverlay();

    if (Skateboard && Skateboard->GetActorLocation().Z < -3000.0f)
    {
        ResetRider();
    }
}

void ASkateCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("SkateSteer"), this, &ASkateCharacter::Steer);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASkateCharacter::TurnCamera);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASkateCharacter::LookUpCamera);

    PlayerInputComponent->BindAction(TEXT("Push"), IE_Pressed, this, &ASkateCharacter::Push);
    PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &ASkateCharacter::BrakePressed);
    PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &ASkateCharacter::BrakeReleased);
    PlayerInputComponent->BindAction(TEXT("Ollie"), IE_Pressed, this, &ASkateCharacter::Ollie);
    PlayerInputComponent->BindAction(TEXT("Kickflip"), IE_Pressed, this, &ASkateCharacter::Kickflip);
    PlayerInputComponent->BindAction(TEXT("ShoveIt"), IE_Pressed, this, &ASkateCharacter::ShoveIt);
    PlayerInputComponent->BindAction(TEXT("ResetRider"), IE_Pressed, this, &ASkateCharacter::ResetRider);
}

void ASkateCharacter::Steer(float Value)
{
    if (Skateboard)
    {
        Skateboard->SetSteering(Value);
    }
}

void ASkateCharacter::TurnCamera(float Value)
{
    AddControllerYawInput(Value);
}

void ASkateCharacter::LookUpCamera(float Value)
{
    AddControllerPitchInput(Value);
}

void ASkateCharacter::Push()
{
    if (Skateboard)
    {
        Skateboard->Push();
    }
}

void ASkateCharacter::BrakePressed()
{
    if (Skateboard)
    {
        Skateboard->SetBraking(true);
    }
}

void ASkateCharacter::BrakeReleased()
{
    if (Skateboard)
    {
        Skateboard->SetBraking(false);
    }
}

void ASkateCharacter::Ollie()
{
    if (Skateboard)
    {
        Skateboard->Ollie();
    }
}

void ASkateCharacter::Kickflip()
{
    if (Skateboard)
    {
        Skateboard->Kickflip();
    }
}

void ASkateCharacter::ShoveIt()
{
    if (Skateboard)
    {
        Skateboard->ShoveIt();
    }
}

void ASkateCharacter::ResetRider()
{
    if (!Skateboard)
    {
        return;
    }

    const FVector ResetLocation = LastSafeBoardLocation + FVector::UpVector * 50.0f;
    const FRotator ResetRotation(0.0f, Skateboard->GetActorRotation().Yaw, 0.0f);

    Skateboard->ResetBoard(ResetLocation, ResetRotation);
    SetActorLocation(ResetLocation + FVector::UpVector * RiderHeight);
}

void ASkateCharacter::SpawnSkateboard()
{
    if (Skateboard || !GetWorld())
    {
        return;
    }

    UClass* SpawnClass = SkateboardClass ? SkateboardClass.Get() : ASkateboardActor::StaticClass();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const FVector SpawnLocation = GetActorLocation() - FVector::UpVector * RiderHeight;
    const FRotator SpawnRotation(0.0f, GetActorRotation().Yaw, 0.0f);

    Skateboard = GetWorld()->SpawnActor<ASkateboardActor>(
        SpawnClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams);

    if (Skateboard)
    {
        LastSafeBoardLocation = Skateboard->GetActorLocation();
    }
}

void ASkateCharacter::FollowSkateboard(float DeltaSeconds)
{
    if (!Skateboard)
    {
        return;
    }

    if (Skateboard->IsGrounded() && !Skateboard->IsBailed())
    {
        LastSafeBoardLocation = Skateboard->GetActorLocation();
    }

    const FVector TargetLocation = Skateboard->GetActorLocation() + FVector::UpVector * RiderHeight;
    const FVector SmoothedLocation = FMath::VInterpTo(
        GetActorLocation(),
        TargetLocation,
        DeltaSeconds,
        CameraFollowSpeed);

    SetActorLocation(SmoothedLocation);

    const FRotator CurrentRotation = GetActorRotation();
    const FRotator TargetRotation(0.0f, Skateboard->GetActorRotation().Yaw, 0.0f);
    SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, CameraFollowSpeed));
}

void ASkateCharacter::UpdateDebugOverlay() const
{
    if (!bShowSkateDebug || !Skateboard || !GEngine)
    {
        return;
    }

    const UEnum* StateEnum = StaticEnum<ESkateMovementState>();
    const FString StateName = StateEnum
        ? StateEnum->GetNameStringByValue(static_cast<int64>(Skateboard->GetMovementState()))
        : TEXT("Unknown");

    const float SpeedKmh = FMath::Abs(Skateboard->GetForwardSpeed()) * 0.036f;
    const FString DebugText = FString::Printf(
        TEXT("Skate | State: %s | Speed: %.1f km/h | Wheels: %d/4"),
        *StateName,
        SpeedKmh,
        Skateboard->GetGroundedWheelCount());

    GEngine->AddOnScreenDebugMessage(98765, 0.0f, FColor::Cyan, DebugText);
}
