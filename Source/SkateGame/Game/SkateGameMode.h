#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SkateGameMode.generated.h"

class UStaticMesh;

UCLASS()
class SKATEGAME_API ASkateGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASkateGameMode();

    virtual void StartPlay() override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prototype")
    bool bSpawnPrototypeArena = true;

    UPROPERTY()
    TObjectPtr<UStaticMesh> PrototypeCube;

private:
    void SpawnPrototypeArena();
};
