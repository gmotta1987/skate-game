#include "Characters/SkateCharacter.h"

#include "Actors/SkateboardActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"

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
    SpawnSkateboard();
}

void ASkateCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FollowSkateboard(DeltaSeconds);

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

void ASkateCharacter::ResetRider()
{
    if (!Skateboard || !Skateboard->GetBoardMesh())
    {
        return;
    }

    UStaticMeshComponent* BoardMesh = Skateboard->GetBoardMesh();
    BoardMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    BoardMesh->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

    const FVector ResetLocation = LastSafeBoardLocation + FVector::UpVector * 50.0f;
    const FRotator ResetRotation(0.0f, Skateboard->GetActorRotation().Yaw, 0.0f);

    Skateboard->SetActorLocationAndRotation(
        ResetLocation,
        ResetRotation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

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

    if (Skateboard->IsGrounded())
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
