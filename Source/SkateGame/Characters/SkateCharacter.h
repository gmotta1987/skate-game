#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SkateCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASkateboardActor;

UCLASS()
class SKATEGAME_API ASkateCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASkateCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skate")
    TSubclassOf<ASkateboardActor> SkateboardClass;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skate")
    TObjectPtr<ASkateboardActor> Skateboard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Rider", meta = (ClampMin = "0.0"))
    float RiderHeight = 92.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Camera", meta = (ClampMin = "0.0"))
    float CameraFollowSpeed = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Debug")
    bool bShowSkateDebug = true;

private:
    void Steer(float Value);
    void TurnCamera(float Value);
    void LookUpCamera(float Value);
    void Push();
    void BrakePressed();
    void BrakeReleased();
    void Ollie();
    void Kickflip();
    void ShoveIt();
    void ResetRider();

    void SpawnSkateboard();
    void FollowSkateboard(float DeltaSeconds);
    void UpdateDebugOverlay() const;

    FVector LastSafeBoardLocation = FVector::ZeroVector;
};
