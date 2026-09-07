#include "Actors/SkateboardActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ASkateboardActor::ASkateboardActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Keep the simulated body at unit scale. Visual meshes are children, which
    // prevents non-uniform deck scaling from distorting wheel/truck positions.
    PhysicsBody = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsBody"));
    SetRootComponent(PhysicsBody);
    PhysicsBody->SetBoxExtent(FVector(40.0f, 11.0f, 2.0f));
    PhysicsBody->SetSimulatePhysics(true);
    PhysicsBody->SetCollisionProfileName(TEXT("PhysicsActor"));
    PhysicsBody->SetLinearDamping(0.05f);
    PhysicsBody->SetAngularDamping(1.2f);
    PhysicsBody->BodyInstance.bUseCCD = true;
    PhysicsBody->SetMassOverrideInKg(NAME_None, 3.0f, true);

    BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
    BoardMesh->SetupAttachment(PhysicsBody);

    FrontTruckMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontTruckMesh"));
    FrontTruckMesh->SetupAttachment(PhysicsBody);

    RearTruckMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearTruckMesh"));
    RearTruckMesh->SetupAttachment(PhysicsBody);

    FrontLeftWheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftWheelMesh"));
    FrontLeftWheelMesh->SetupAttachment(PhysicsBody);

    FrontRightWheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightWheelMesh"));
    FrontRightWheelMesh->SetupAttachment(PhysicsBody);

    RearLeftWheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearLeftWheelMesh"));
    RearLeftWheelMesh->SetupAttachment(PhysicsBody);

    RearRightWheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearRightWheelMesh"));
    RearRightWheelMesh->SetupAttachment(PhysicsBody);

    ConfigurePrototypeVisuals();
    RebuildWheelOffsets();
}

void ASkateboardActor::BeginPlay()
{
    Super::BeginPlay();

    RebuildWheelOffsets();
    UpdateWheelContacts();
    UpdatePrototypeWheelVisuals();
    EnterMovementState(bGrounded ? ESkateMovementState::Grounded : ESkateMovementState::Airborne);
}

void ASkateboardActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!PhysicsBody || DeltaSeconds <= 0.0f)
    {
        return;
    }

    TimeInMovementState += DeltaSeconds;

    if (MovementState == ESkateMovementState::Airborne || MovementState == ESkateMovementState::Pop)
    {
        LastAirborneVerticalSpeed = PhysicsBody->GetPhysicsLinearVelocity().Z;
    }

    UpdateWheelContacts();
    UpdatePrototypeWheelVisuals();
    ApplySuspensionForces();
    UpdateMovementState(DeltaSeconds);
    ApplyRollingForces(DeltaSeconds);
    ApplySteering(DeltaSeconds);
    ClampPlanarSpeed();
}

void ASkateboardActor::Push(float Strength)
{
    if (!bGrounded || MovementState == ESkateMovementState::Bail || !PhysicsBody)
    {
        return;
    }

    const float ClampedStrength = FMath::Clamp(Strength, 0.0f, 1.5f);
    const FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
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

    // Velocity-change mode keeps tuning predictable if total board mass changes.
    PhysicsBody->AddImpulse(Forward * PushImpulse * ClampedStrength, NAME_None, true);
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
    if (!CanPop() || !PhysicsBody)
    {
        return;
    }

    const float ClampedStrength = FMath::Clamp(Strength, 0.25f, 1.5f);
    const FVector PopDirection = (GroundNormal * 0.75f + FVector::UpVector * 0.25f).GetSafeNormal();

    PhysicsBody->AddImpulse(PopDirection * OllieImpulse * ClampedStrength, NAME_None, true);
    LastAirborneVerticalSpeed = PhysicsBody->GetPhysicsLinearVelocity().Z;
    EnterMovementState(ESkateMovementState::Pop);
}

void ASkateboardActor::Kickflip(float Direction)
{
    if (!CanPop() || !PhysicsBody)
    {
        return;
    }

    const float TrickDirection = FMath::IsNearlyZero(Direction) ? 1.0f : FMath::Sign(Direction);
    Ollie(0.95f);

    PhysicsBody->AddAngularImpulseInRadians(
        GetActorForwardVector().GetSafeNormal() * FlipAngularImpulse * TrickDirection,
        NAME_None,
        true);
}

void ASkateboardActor::ShoveIt(float Direction)
{
    if (!CanPop() || !PhysicsBody)
    {
        return;
    }

    const float TrickDirection = FMath::IsNearlyZero(Direction) ? 1.0f : FMath::Sign(Direction);
    Ollie(0.90f);

    const FVector SpinAxis = GroundNormal.IsNearlyZero() ? FVector::UpVector : GroundNormal.GetSafeNormal();
    PhysicsBody->AddAngularImpulseInRadians(
        SpinAxis * ShoveAngularImpulse * TrickDirection,
        NAME_None,
        true);
}

void ASkateboardActor::ResetBoard(FVector NewLocation, FRotator NewRotation)
{
    if (!PhysicsBody)
    {
        return;
    }

    PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

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
    UpdatePrototypeWheelVisuals();

    if (bGrounded)
    {
        EnterMovementState(ESkateMovementState::Grounded);
    }
}

float ASkateboardActor::GetForwardSpeed() const
{
    if (!PhysicsBody)
    {
        return 0.0f;
    }

    return FVector::DotProduct(PhysicsBody->GetPhysicsLinearVelocity(), GetActorForwardVector());
}

void ASkateboardActor::ConfigurePrototypeVisuals()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    UStaticMeshComponent* CubeVisuals[] =
    {
        BoardMesh,
        FrontTruckMesh,
        RearTruckMesh
    };

    for (UStaticMeshComponent* Component : CubeVisuals)
    {
        if (!Component)
        {
            continue;
        }

        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);

        if (CubeMesh.Succeeded())
        {
            Component->SetStaticMesh(CubeMesh.Object);
        }
    }

    if (BoardMesh)
    {
        BoardMesh->SetRelativeScale3D(FVector(0.80f, 0.22f, 0.04f));
    }

    if (FrontTruckMesh)
    {
        FrontTruckMesh->SetRelativeScale3D(FVector(0.055f, 0.19f, 0.015f));
    }

    if (RearTruckMesh)
    {
        RearTruckMesh->SetRelativeScale3D(FVector(0.055f, 0.19f, 0.015f));
    }

    UStaticMeshComponent* WheelVisuals[] =
    {
        FrontLeftWheelMesh,
        FrontRightWheelMesh,
        RearLeftWheelMesh,
        RearRightWheelMesh
    };

    for (UStaticMeshComponent* Wheel : WheelVisuals)
    {
        if (!Wheel)
        {
            continue;
        }

        Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Wheel->SetGenerateOverlapEvents(false);
        Wheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
        Wheel->SetRelativeScale3D(FVector(0.056f, 0.056f, 0.032f));

        if (CylinderMesh.Succeeded())
        {
            Wheel->SetStaticMesh(CylinderMesh.Object);
        }
    }
}

void ASkateboardActor::RebuildWheelOffsets()
{
    const float HalfWheelbase = Wheelbase * 0.5f;
    const float HalfTrackWidth = TrackWidth * 0.5f;

    WheelLocalOffsets[0] = FVector(HalfWheelbase, -HalfTrackWidth, WheelAnchorHeight);
    WheelLocalOffsets[1] = FVector(HalfWheelbase, HalfTrackWidth, WheelAnchorHeight);
    WheelLocalOffsets[2] = FVector(-HalfWheelbase, -HalfTrackWidth, WheelAnchorHeight);
    WheelLocalOffsets[3] = FVector(-HalfWheelbase, HalfTrackWidth, WheelAnchorHeight);

    if (FrontTruckMesh)
    {
        FrontTruckMesh->SetRelativeLocation(FVector(HalfWheelbase, 0.0f, -2.8f));
    }

    if (RearTruckMesh)
    {
        RearTruckMesh->SetRelativeLocation(FVector(-HalfWheelbase, 0.0f, -2.8f));
    }
}

FVector ASkateboardActor::GetWheelAnchorWorldLocation(int32 WheelIndex) const
{
    if (WheelIndex < 0 || WheelIndex >= WheelCount)
    {
        return GetActorLocation();
    }

    const FVector& LocalOffset = WheelLocalOffsets[WheelIndex];

    return GetActorLocation()
        + GetActorForwardVector() * LocalOffset.X
        + GetActorRightVector() * LocalOffset.Y
        + GetActorUpVector() * LocalOffset.Z;
}

UStaticMeshComponent* ASkateboardActor::GetWheelVisual(int32 WheelIndex) const
{
    switch (WheelIndex)
    {
        case 0: return FrontLeftWheelMesh;
        case 1: return FrontRightWheelMesh;
        case 2: return RearLeftWheelMesh;
        case 3: return RearRightWheelMesh;
        default: return nullptr;
    }
}

void ASkateboardActor::UpdatePrototypeWheelVisuals()
{
    const FVector BoardUp = GetActorUpVector().GetSafeNormal();

    for (int32 WheelIndex = 0; WheelIndex < WheelCount; ++WheelIndex)
    {
        UStaticMeshComponent* Wheel = GetWheelVisual(WheelIndex);
        if (!Wheel)
        {
            continue;
        }

        float VisualSuspensionLength = SuspensionRestLength;
        const FHitResult& Hit = WheelHits[WheelIndex];

        if (Hit.bBlockingHit)
        {
            const FVector Anchor = GetWheelAnchorWorldLocation(WheelIndex);
            const float ActualWheelDistance = FVector::DotProduct(Anchor - Hit.ImpactPoint, BoardUp);
            VisualSuspensionLength = FMath::Clamp(
                ActualWheelDistance - WheelRadius,
                0.0f,
                SuspensionRestLength);
        }

        FVector VisualOffset = WheelLocalOffsets[WheelIndex];
        VisualOffset.Z -= VisualSuspensionLength;
        Wheel->SetRelativeLocation(VisualOffset);
    }
}

void ASkateboardActor::UpdateWheelContacts()
{
    if (!GetWorld() || !PhysicsBody)
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
    if (!PhysicsBody || GroundedWheelCount <= 0)
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

        const FVector PointVelocity = PhysicsBody->GetPhysicsLinearVelocityAtPoint(Anchor);
        const float NormalSpeed = FVector::DotProduct(PointVelocity, Hit.ImpactNormal);

        const float SpringForce = Compression * SuspensionStrength;
        const float DampingForce = -NormalSpeed * SuspensionDamping;
        const float ForceMagnitude = FMath::Max(0.0f, SpringForce + DampingForce);

        PhysicsBody->AddForceAtLocation(
            Hit.ImpactNormal * ForceMagnitude,
            Anchor,
            NAME_None);
    }
}

void ASkateboardActor::ApplyRollingForces(float DeltaSeconds)
{
    if (GroundedWheelCount <= 0 || !PhysicsBody || DeltaSeconds <= 0.0f || MovementState == ESkateMovementState::Bail)
    {
        return;
    }

    const FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
    const FVector PlanarVelocity = FVector::VectorPlaneProject(Velocity, GroundNormal);

    if (!PlanarVelocity.IsNearlyZero())
    {
        const FVector Right = FVector::VectorPlaneProject(GetActorRightVector(), GroundNormal).GetSafeNormal();
        const float LateralSpeed = FVector::DotProduct(PlanarVelocity, Right);
        const FVector LateralVelocity = Right * LateralSpeed;

        FVector Acceleration = -PlanarVelocity * RollingResistance;
        Acceleration += -LateralVelocity * GroundGrip;

        if (bBraking)
        {
            Acceleration += -PlanarVelocity * BrakeDrag;
        }

        PhysicsBody->AddForce(Acceleration, NAME_None, true);
    }

    // Self-righting is deliberately disabled during pop/air so flip tricks can
    // rotate freely instead of fighting the gameplay impulse.
    if (MovementState == ESkateMovementState::Grounded || MovementState == ESkateMovementState::Landing)
    {
        const FVector UprightAxis = FVector::CrossProduct(GetActorUpVector(), GroundNormal);
        PhysicsBody->AddTorqueInRadians(UprightAxis * SelfRightingStrength, NAME_None, true);
    }
}

void ASkateboardActor::ApplySteering(float DeltaSeconds)
{
    if (!bGrounded || !PhysicsBody || FMath::IsNearlyZero(SteeringInput) || DeltaSeconds <= 0.0f || MovementState == ESkateMovementState::Bail)
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

    PhysicsBody->AddTorqueInRadians(
        SteeringAxis * SteeringInput * SteeringTorque * SpeedScale,
        NAME_None,
        true);
}

void ASkateboardActor::ClampPlanarSpeed()
{
    if (!PhysicsBody || MaxSpeed <= 0.0f)
    {
        return;
    }

    const FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
    FVector PlanarVelocity(Velocity.X, Velocity.Y, 0.0f);

    if (PlanarVelocity.SizeSquared() <= FMath::Square(MaxSpeed))
    {
        return;
    }

    PlanarVelocity = PlanarVelocity.GetSafeNormal() * MaxSpeed;
    PhysicsBody->SetPhysicsLinearVelocity(FVector(PlanarVelocity.X, PlanarVelocity.Y, Velocity.Z));
}

void ASkateboardActor::UpdateMovementState(float DeltaSeconds)
{
    if (!PhysicsBody || DeltaSeconds <= 0.0f)
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
    return PhysicsBody
        && bGrounded
        && MovementState != ESkateMovementState::Bail
        && (MovementState == ESkateMovementState::Grounded || MovementState == ESkateMovementState::Landing);
}
