#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SkateCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> PushAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> BrakeAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> OllieAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skate")
    TSubclassOf<ASkateboardActor> SkateboardClass;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skate")
    TObjectPtr<ASkateboardActor> Skateboard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Handling", meta = (ClampMin = "0.0"))
    float SteeringRate = 95.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate|Handling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AirSteeringScale = 0.35f;

private:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Push(const FInputActionValue& Value);
    void BrakeStarted(const FInputActionValue& Value);
    void BrakeCompleted(const FInputActionValue& Value);
    void Ollie(const FInputActionValue& Value);

    void SpawnAndAttachSkateboard();
};
