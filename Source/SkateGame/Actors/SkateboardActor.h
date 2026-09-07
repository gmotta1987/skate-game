#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkateboardActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESkateMovementState : uint8
{
    Grounded UMETA(DisplayName = "Grounded"),
    Pop UMETA(DisplayName = "Pop"),
    Airborne UMETA(DisplayName = "Airborne"),
    Landing UMETA(DisplayName = "Landing"),
    Bail UMETA(DisplayName = "Bail")
};

UCLASS()
class SKATEGAME_API ASkateboardActor : public AActor
{
    GENERATED_BODY()

public:
    ASkateboardActor();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Skate")
    void Push(float Strength = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Skate")
    void SetSteering(float InSteeringInput);

    UFUNCTION(BlueprintCallable, Category = "Skate")
    void SetBraking(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Skate|Tricks")
    void Ollie(float Strength = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Skate|Tricks")
    void Kickflip(float Direction = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Skate|Tricks")
    void ShoveIt(float Direction = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Skate")
    void ResetBoard(FVector NewLocation, FRotator NewRotation);

    UFUNCTION(BlueprintPure, Category = "Skate")
    bool IsGrounded() const { return bGrounded; }

    UFUNCTION(BlueprintPure, Category = "Skate")
    bool IsBailed() const { return MovementState == ESkateMovementState::Bail; }

    UFUNCTION(BlueprintPure, Category = "Skate")
    ESkateMovementState GetMovementState() const { return MovementState; }

    UFUNCTION(BlueprintPure, Category = "Skate")
    int32 GetGroundedWheelCount() const { return GroundedWheelCount; }

    UFUNCTION(BlueprintPure, Category = "Skate")
    float GetForwardSpeed() const;

    UFUNCTION(BlueprintPure, Category = "Skate")
    UStaticMeshComponent* GetBoardMesh() const { return BoardMesh; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Physics")
    TObjectPtr<UBoxComponent> PhysicsBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Visual")
    TObjectPtr<UStaticMeshComponent> BoardMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Visual")
    TObjectPtr<UStaticMeshComponent> FrontTruckMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Visual")
    TObjectPtr<UStaticMeshComponent> RearTruckMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Visual")
    TObjectPtr<UStaticMeshComponent> FrontLeftWheelMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Visual")
    TObjectPtr<UStaticMeshComponent> FrontRightWheelMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Visual")
    TObjectPtr<UStaticMeshComponent> RearLeftWheelMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate|Visual")
    TObjectPtr<UStaticMeshComponent> RearRightWheelMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float PushImpulse = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float MaxSpeed = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float RollingResistance = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float BrakeDrag = 4.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float GroundGrip = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float OllieImpulse = 430.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float SteeringTorque = 3.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "0.0"))
    float Wheelbase = 52.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "0.0"))
    float TrackWidth = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels")
    float WheelAnchorHeight = -3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "0.0"))
    float WheelRadius = 2.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "0.0"))
    float SuspensionRestLength = 1.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "0.0"))
    float WheelTraceStartOffset = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "0.0"))
    float SuspensionStrength = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "0.0"))
    float SuspensionDamping = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Wheels", meta = (ClampMin = "1", ClampMax = "4"))
    int32 MinGroundedWheels = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Stability", meta = (ClampMin = "0.0"))
    float SelfRightingStrength = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Landing", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MaxLandingTiltDegrees = 48.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Landing", meta = (ClampMin = "0.0"))
    float MaxLandingVerticalSpeed = 1050.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Landing", meta = (ClampMin = "0.0"))
    float LandingSettleTime = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Tricks", meta = (ClampMin = "0.0"))
    float PopStateDuration = 0.09f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Tricks", meta = (ClampMin = "0.0"))
    float FlipAngularImpulse = 10.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Tricks", meta = (ClampMin = "0.0"))
    float ShoveAngularImpulse = 7.5f;

private:
    static constexpr int32 WheelCount = 4;

    FVector WheelLocalOffsets[WheelCount];
    FHitResult WheelHits[WheelCount];

    float SteeringInput = 0.0f;
    bool bBraking = false;
    bool bGrounded = false;
    int32 GroundedWheelCount = 0;
    FVector GroundNormal = FVector::UpVector;

    ESkateMovementState MovementState = ESkateMovementState::Airborne;
    float TimeInMovementState = 0.0f;
    float LastAirborneVerticalSpeed = 0.0f;

    void RebuildWheelOffsets();
    void ConfigurePrototypeVisuals();
    void UpdatePrototypeWheelVisuals();
    void UpdateWheelContacts();
    void ApplySuspensionForces();
    void ApplyRollingForces(float DeltaSeconds);
    void ApplySteering(float DeltaSeconds);
    void ClampPlanarSpeed();
    void UpdateMovementState(float DeltaSeconds);
    void EnterMovementState(ESkateMovementState NewState);
    bool CanPop() const;
    FVector GetWheelAnchorWorldLocation(int32 WheelIndex) const;
    UStaticMeshComponent* GetWheelVisual(int32 WheelIndex) const;
};
