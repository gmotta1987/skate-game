#include "Actors/SkateboardActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ASkateboardActor::ASkateboardActor()
{
    PrimaryActorTick.bCanEverTick = true;

    BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
    SetRootComponent(BoardMesh);

    BoardMesh->SetSimulatePhysics(true);
    BoardMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    BoardMesh->SetLinearDamping(0.05f);
    BoardMesh->SetAngularDamping(2.0f);
    BoardMesh->BodyInstance.bUseCCD = true;
    BoardMesh->SetMassOverrideInKg(NAME_None, 3.0f, true);

    // Temporary fallback so the prototype is playable before final art is imported.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        BoardMesh->SetStaticMesh(CubeMesh.Object);
        BoardMesh->SetRelativeScale3D(FVector(0.80f, 0.22f, 0.04f));
    }
}

void ASkateboardActor::BeginPlay()
{
    Super::BeginPlay();
    UpdateGroundState();
}

void ASkateboardActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateGroundState();
    ApplyRollingForces(DeltaSeconds);
    ApplySteering(DeltaSeconds);
    ClampPlanarSpeed();
}

void ASkateboardActor::Push(float Strength)
{
    if (!bGrounded || !BoardMesh)
    {
        return;
    }

    const float ClampedStrength = FMath::Clamp(Strength, 0.0f, 1.5f);
    const FVector Velocity = BoardMesh->GetPhysicsLinearVelocity();
    const FVector PlanarVelocity(Velocity.X, Velocity.Y, 0.0f);

    if (PlanarVelocity.Size() >= MaxSpeed)
    {
        return;
    }

    FVector Forward = GetActorForwardVector();
    Forward.Z = 0.0f;
    Forward.Normalize();

    // Velocity-change mode keeps the tuning predictable if board mass changes.
    BoardMesh->AddImpulse(Forward * PushImpulse * ClampedStrength, NAME_None, true);
}

void ASkateboardActor::SetSteering(float InSteeringInput)
{
    SteeringInput = FMath::Clamp(InSteeringInput, -1.0f, 1.0f);
}

void ASkateboardActor::SetBraking(bool bEnabled)
{
    bBraking = bEnabled;
}

void ASkateboardActor::Ollie(float Strength)
{
    if (!bGrounded || !BoardMesh)
    {
        return;
    }

    const float ClampedStrength = FMath::Clamp(Strength, 0.25f, 1.5f);
    BoardMesh->AddImpulse(FVector::UpVector * OllieImpulse * ClampedStrength, NAME_None, true);
    bGrounded = false;
}

float ASkateboardActor::GetForwardSpeed() const
{
    if (!BoardMesh)
    {
        return 0.0f;
    }

    return FVector::DotProduct(BoardMesh->GetPhysicsLinearVelocity(), GetActorForwardVector());
}

void ASkateboardActor::UpdateGroundState()
{
    if (!GetWorld() || !BoardMesh)
    {
        bGrounded = false;
        return;
    }

    const FVector Start = BoardMesh->GetComponentLocation() + FVector::UpVector * 6.0f;
    const FVector End = Start - FVector::UpVector * GroundTraceDistance;

    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkateGroundTrace), false, this);
    QueryParams.AddIgnoredActor(this);

    bGrounded = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        QueryParams);
}

void ASkateboardActor::ApplyRollingForces(float DeltaSeconds)
{
    if (!bGrounded || !BoardMesh || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const FVector Velocity = BoardMesh->GetPhysicsLinearVelocity();
    const FVector PlanarVelocity(Velocity.X, Velocity.Y, 0.0f);

    if (PlanarVelocity.IsNearlyZero())
    {
        return;
    }

    const FVector Right = GetActorRightVector().GetSafeNormal2D();
    const float LateralSpeed = FVector::DotProduct(PlanarVelocity, Right);
    const FVector LateralVelocity = Right * LateralSpeed;

    // Rolling drag keeps momentum, while lateral grip prevents the board from
    // behaving like a frictionless puck.
    FVector Acceleration = -PlanarVelocity * RollingResistance;
    Acceleration += -LateralVelocity * GroundGrip;

    if (bBraking)
    {
        Acceleration += -PlanarVelocity * BrakeDrag;
    }

    BoardMesh->AddForce(Acceleration, NAME_None, true);

    // Soft self-righting for the initial prototype. This will later be relaxed
    // while flip tricks are active.
    const FVector UprightAxis = FVector::CrossProduct(GetActorUpVector(), FVector::UpVector);
    BoardMesh->AddTorqueInRadians(UprightAxis * 5.0f, NAME_None, true);
}

void ASkateboardActor::ApplySteering(float DeltaSeconds)
{
    if (!bGrounded || !BoardMesh || FMath::IsNearlyZero(SteeringInput) || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const float Speed = FMath::Abs(GetForwardSpeed());
    if (Speed < 40.0f)
    {
        return;
    }

    const float SpeedScale = FMath::Clamp(Speed / 700.0f, 0.25f, 1.0f);
    BoardMesh->AddTorqueInRadians(
        FVector::UpVector * SteeringInput * SteeringTorque * SpeedScale,
        NAME_None,
        true);
}

void ASkateboardActor::ClampPlanarSpeed()
{
    if (!BoardMesh || MaxSpeed <= 0.0f)
    {
        return;
    }

    const FVector Velocity = BoardMesh->GetPhysicsLinearVelocity();
    FVector PlanarVelocity(Velocity.X, Velocity.Y, 0.0f);

    if (PlanarVelocity.SizeSquared() <= FMath::Square(MaxSpeed))
    {
        return;
    }

    PlanarVelocity = PlanarVelocity.GetSafeNormal() * MaxSpeed;
    BoardMesh->SetPhysicsLinearVelocity(FVector(PlanarVelocity.X, PlanarVelocity.Y, Velocity.Z));
}
