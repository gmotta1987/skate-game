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
    BoardMesh->SetAngularDamping(1.2f);
    BoardMesh->BodyInstance.bUseCCD = true;
    BoardMesh->SetMassOverrideInKg(NAME_None, 3.0f, true);

    // Temporary fallback so the prototype is playable before final art is imported.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        BoardMesh->SetStaticMesh(CubeMesh.Object);
        BoardMesh->SetRelativeScale3D(FVector(0.80f, 0.22f, 0.04f));
    }

    RebuildWheelOffsets();
}

void ASkateboardActor::BeginPlay()
{
    Super::BeginPlay();

    RebuildWheelOffsets();
    UpdateWheelContacts();
    EnterMovementState(bGrounded ? ESkateMovementState::Grounded : ESkateMovementState::Airborne);
}

void ASkateboardActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!BoardMesh || DeltaSeconds <= 0.0f)
    {
        return;
    }

    TimeInMovementState += DeltaSeconds;

    if (MovementState == ESkateMovementState::Airborne || MovementState == ESkateMovementState::Pop)
    {
        LastAirborneVerticalSpeed = BoardMesh->GetPhysicsLinearVelocity().Z;
    }

    UpdateWheelContacts();
    ApplySuspensionForces();
    UpdateMovementState(DeltaSeconds);
    ApplyRollingForces(DeltaSeconds);
    ApplySteering(DeltaSeconds);
    ClampPlanarSpeed();
}

void ASkateboardActor::Push(float Strength)
{
    if (!bGrounded || MovementState == ESkateMovementState::Bail || !BoardMesh)
    {
        return;
    }

    const float ClampedStrength = FMath::Clamp(Strength, 0.0f, 1.5f);
    const FVector Velocity = BoardMesh->GetPhysicsLinearVelocity();
    const FVector PlanarVelocity = FVector::VectorPlaneProject(Velocity, GroundNormal);

    if (PlanarVelocity.Size() >= MaxSpeed)
    {
        return;
    }

    FVector Forward = FVector::VectorPlaneProject(GetActorForwardVector(), GroundNormal).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = GetActorForwardVector().GetSafeNormal2D();
    }

    // Velocity-change mode keeps tuning predictable if the board mass changes.
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
    if (!CanPop() || !BoardMesh)
    {
        return;
    }

    const float ClampedStrength = FMath::Clamp(Strength, 0.25f, 1.5f);
    const FVector PopDirection = (GroundNormal * 0.75f + FVector::UpVector * 0.25f).GetSafeNormal();

    BoardMesh->AddImpulse(PopDirection * OllieImpulse * ClampedStrength, NAME_None, true);
    LastAirborneVerticalSpeed = BoardMesh->GetPhysicsLinearVelocity().Z;
    EnterMovementState(ESkateMovementState::Pop);
}

void ASkateboardActor::Kickflip(float Direction)
{
    if (!CanPop() || !BoardMesh)
    {
        return;
    }

    const float TrickDirection = FMath::IsNearlyZero(Direction) ? 1.0f : FMath::Sign(Direction);
    Ollie(0.95f);

    BoardMesh->AddAngularImpulseInRadians(
        GetActorForwardVector().GetSafeNormal() * FlipAngularImpulse * TrickDirection,
        NAME_None,
        true);
}

void ASkateboardActor::ShoveIt(float Direction)
{
    if (!CanPop() || !BoardMesh)
    {
        return;
    }

    const float TrickDirection = FMath::IsNearlyZero(Direction) ? 1.0f : FMath::Sign(Direction);
    Ollie(0.90f);

    const FVector SpinAxis = GroundNormal.IsNearlyZero() ? FVector::UpVector : GroundNormal.GetSafeNormal();
    BoardMesh->AddAngularImpulseInRadians(
        SpinAxis * ShoveAngularImpulse * TrickDirection,
        NAME_None,
        true);
}

void ASkateboardActor::ResetBoard(FVector NewLocation, FRotator NewRotation)
{
    if (!BoardMesh)
    {
        return;
    }

    BoardMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    BoardMesh->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

    SetActorLocationAndRotation(
        NewLocation,
        NewRotation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    SteeringInput = 0.0f;
    bBraking = false;
    bGrounded = false;
    GroundedWheelCount = 0;
    GroundNormal = FVector::UpVector;
    LastAirborneVerticalSpeed = 0.0f;

    EnterMovementState(ESkateMovementState::Airborne);
    UpdateWheelContacts();

    if (bGrounded)
    {
        EnterMovementState(ESkateMovementState::Grounded);
    }
}

float ASkateboardActor::GetForwardSpeed() const
{
    if (!BoardMesh)
    {
        return 0.0f;
    }

    return FVector::DotProduct(BoardMesh->GetPhysicsLinearVelocity(), GetActorForwardVector());
}

void ASkateboardActor::RebuildWheelOffsets()
{
    const float HalfWheelbase = Wheelbase * 0.5f;
    const float HalfTrackWidth = TrackWidth * 0.5f;

    WheelLocalOffsets[0] = FVector(HalfWheelbase, -HalfTrackWidth, WheelAnchorHeight);
    WheelLocalOffsets[1] = FVector(HalfWheelbase, HalfTrackWidth, WheelAnchorHeight);
    WheelLocalOffsets[2] = FVector(-HalfWheelbase, -HalfTrackWidth, WheelAnchorHeight);
    WheelLocalOffsets[3] = FVector(-HalfWheelbase, HalfTrackWidth, WheelAnchorHeight);
}

FVector ASkateboardActor::GetWheelAnchorWorldLocation(int32 WheelIndex) const
{
    if (WheelIndex < 0 || WheelIndex >= WheelCount)
    {
        return GetActorLocation();
    }

    const FVector& LocalOffset = WheelLocalOffsets[WheelIndex];

    // Build the offset from the actor basis instead of TransformPosition so the
    // temporary cube mesh scale does not shrink the virtual wheelbase/track.
    return GetActorLocation()
        + GetActorForwardVector() * LocalOffset.X
        + GetActorRightVector() * LocalOffset.Y
        + GetActorUpVector() * LocalOffset.Z;
}

void ASkateboardActor::UpdateWheelContacts()
{
    if (!GetWorld() || !BoardMesh)
    {
        bGrounded = false;
        GroundedWheelCount = 0;
        GroundNormal = FVector::UpVector;
        return;
    }

    GroundedWheelCount = 0;
    FVector AccumulatedNormal = FVector::ZeroVector;

    const FVector BoardUp = GetActorUpVector().GetSafeNormal();
    const float TraceDistance = SuspensionRestLength + WheelRadius;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkateWheelTrace), false, this);
    QueryParams.AddIgnoredActor(this);
    if (GetOwner())
    {
        QueryParams.AddIgnoredActor(GetOwner());
    }

    for (int32 WheelIndex = 0; WheelIndex < WheelCount; ++WheelIndex)
    {
        WheelHits[WheelIndex] = FHitResult();

        const FVector Anchor = GetWheelAnchorWorldLocation(WheelIndex);
        const FVector Start = Anchor + BoardUp * WheelTraceStartOffset;
        const FVector End = Anchor - BoardUp * TraceDistance;

        const bool bHit = GetWorld()->LineTraceSingleByChannel(
            WheelHits[WheelIndex],
            Start,
            End,
            ECC_Visibility,
            QueryParams);

        if (bHit && WheelHits[WheelIndex].bBlockingHit)
        {
            ++GroundedWheelCount;
            AccumulatedNormal += WheelHits[WheelIndex].ImpactNormal;
        }
    }

    bGrounded = GroundedWheelCount >= FMath::Clamp(MinGroundedWheels, 1, WheelCount);
    GroundNormal = GroundedWheelCount > 0
        ? AccumulatedNormal.GetSafeNormal()
        : FVector::UpVector;
}

void ASkateboardActor::ApplySuspensionForces()
{
    if (!BoardMesh || GroundedWheelCount <= 0)
    {
        return;
    }

    const FVector BoardUp = GetActorUpVector().GetSafeNormal();
    const float TargetWheelDistance = SuspensionRestLength + WheelRadius;

    for (int32 WheelIndex = 0; WheelIndex < WheelCount; ++WheelIndex)
    {
        const FHitResult& Hit = WheelHits[WheelIndex];
        if (!Hit.bBlockingHit)
        {
            continue;
        }

        const FVector Anchor = GetWheelAnchorWorldLocation(WheelIndex);
        const float ActualWheelDistance = FVector::DotProduct(Anchor - Hit.ImpactPoint, BoardUp);
        const float Compression = FMath::Clamp(
            TargetWheelDistance - ActualWheelDistance,
            0.0f,
            SuspensionRestLength);

        if (Compression <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const FVector PointVelocity = BoardMesh->GetPhysicsLinearVelocityAtPoint(Anchor);
        const float NormalSpeed = FVector::DotProduct(PointVelocity, Hit.ImpactNormal);

        const float SpringForce = Compression * SuspensionStrength;
        const float DampingForce = -NormalSpeed * SuspensionDamping;
        const float ForceMagnitude = FMath::Max(0.0f, SpringForce + DampingForce);

        BoardMesh->AddForceAtLocation(
            Hit.ImpactNormal * ForceMagnitude,
            Anchor,
            NAME_None);
    }
}

void ASkateboardActor::ApplyRollingForces(float DeltaSeconds)
{
    if (GroundedWheelCount <= 0 || !BoardMesh || DeltaSeconds <= 0.0f || MovementState == ESkateMovementState::Bail)
    {
        return;
    }

    const FVector Velocity = BoardMesh->GetPhysicsLinearVelocity();
    const FVector PlanarVelocity = FVector::VectorPlaneProject(Velocity, GroundNormal);

    if (!PlanarVelocity.IsNearlyZero())
    {
        const FVector Right = FVector::VectorPlaneProject(GetActorRightVector(), GroundNormal).GetSafeNormal();
        const float LateralSpeed = FVector::DotProduct(PlanarVelocity, Right);
        const FVector LateralVelocity = Right * LateralSpeed;

        // Rolling drag preserves momentum, while lateral grip keeps the board
        // from behaving like a frictionless puck.
        FVector Acceleration = -PlanarVelocity * RollingResistance;
        Acceleration += -LateralVelocity * GroundGrip;

        if (bBraking)
        {
            Acceleration += -PlanarVelocity * BrakeDrag;
        }

        BoardMesh->AddForce(Acceleration, NAME_None, true);
    }

    // Grounded self-righting is deliberately disabled during pop/air so flip
    // tricks can rotate freely.
    if (MovementState == ESkateMovementState::Grounded || MovementState == ESkateMovementState::Landing)
    {
        const FVector UprightAxis = FVector::CrossProduct(GetActorUpVector(), GroundNormal);
        BoardMesh->AddTorqueInRadians(UprightAxis * SelfRightingStrength, NAME_None, true);
    }
}

void ASkateboardActor::ApplySteering(float DeltaSeconds)
{
    if (!bGrounded || !BoardMesh || FMath::IsNearlyZero(SteeringInput) || DeltaSeconds <= 0.0f || MovementState == ESkateMovementState::Bail)
    {
        return;
    }

    const float Speed = FMath::Abs(GetForwardSpeed());
    if (Speed < 40.0f)
    {
        return;
    }

    const float SpeedScale = FMath::Clamp(Speed / 700.0f, 0.25f, 1.0f);
    const FVector SteeringAxis = GroundNormal.IsNearlyZero() ? FVector::UpVector : GroundNormal.GetSafeNormal();

    BoardMesh->AddTorqueInRadians(
        SteeringAxis * SteeringInput * SteeringTorque * SpeedScale,
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

void ASkateboardActor::UpdateMovementState(float DeltaSeconds)
{
    if (!BoardMesh || DeltaSeconds <= 0.0f)
    {
        return;
    }

    switch (MovementState)
    {
        case ESkateMovementState::Grounded:
        {
            if (!bGrounded)
            {
                EnterMovementState(ESkateMovementState::Airborne);
            }
            break;
        }

        case ESkateMovementState::Pop:
        {
            if (!bGrounded || TimeInMovementState >= PopStateDuration)
            {
                EnterMovementState(ESkateMovementState::Airborne);
            }
            break;
        }

        case ESkateMovementState::Airborne:
        {
            if (bGrounded)
            {
                const float LandingImpactSpeed = FMath::Max(0.0f, -LastAirborneVerticalSpeed);
                const float Alignment = FVector::DotProduct(
                    GetActorUpVector().GetSafeNormal(),
                    GroundNormal.GetSafeNormal());
                const float MinLandingAlignment = FMath::Cos(FMath::DegreesToRadians(MaxLandingTiltDegrees));

                const bool bHardLanding = LandingImpactSpeed > MaxLandingVerticalSpeed;
                const bool bBadBoardAngle = Alignment < MinLandingAlignment;

                EnterMovementState(
                    (bHardLanding || bBadBoardAngle)
                        ? ESkateMovementState::Bail
                        : ESkateMovementState::Landing);
            }
            break;
        }

        case ESkateMovementState::Landing:
        {
            if (!bGrounded)
            {
                EnterMovementState(ESkateMovementState::Airborne);
            }
            else if (TimeInMovementState >= LandingSettleTime)
            {
                EnterMovementState(ESkateMovementState::Grounded);
            }
            break;
        }

        case ESkateMovementState::Bail:
        default:
            break;
    }
}

void ASkateboardActor::EnterMovementState(ESkateMovementState NewState)
{
    if (MovementState == NewState)
    {
        return;
    }

    MovementState = NewState;
    TimeInMovementState = 0.0f;
}

bool ASkateboardActor::CanPop() const
{
    return BoardMesh
        && bGrounded
        && MovementState != ESkateMovementState::Bail
        && (MovementState == ESkateMovementState::Grounded || MovementState == ESkateMovementState::Landing);
}
