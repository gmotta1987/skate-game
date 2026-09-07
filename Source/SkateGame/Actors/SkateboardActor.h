#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkateboardActor.generated.h"

class UStaticMeshComponent;

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
    void SetSteering(float SteeringInput);

    UFUNCTION(BlueprintCallable, Category = "Skate")
    void SetBraking(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Skate")
    void Ollie(float Strength = 1.0f);

    UFUNCTION(BlueprintPure, Category = "Skate")
    bool IsGrounded() const { return bGrounded; }

    UFUNCTION(BlueprintPure, Category = "Skate")
    float GetForwardSpeed() const;

    UFUNCTION(BlueprintPure, Category = "Skate")
    UStaticMeshComponent* GetBoardMesh() const { return BoardMesh; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate")
    TObjectPtr<UStaticMeshComponent> BoardMesh;

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
    float GroundTraceDistance = 28.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Physics", meta = (ClampMin = "0.0"))
    float SteeringTorque = 3.5f;

private:
    float SteeringInput = 0.0f;
    bool bBraking = false;
    bool bGrounded = false;

    void UpdateGroundState();
    void ApplyRollingForces(float DeltaSeconds);
    void ApplySteering(float DeltaSeconds);
    void ClampPlanarSpeed();
};
