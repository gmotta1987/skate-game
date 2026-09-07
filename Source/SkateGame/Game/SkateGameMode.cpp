#include "Game/SkateGameMode.h"

#include "Characters/SkateCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
AStaticMeshActor* SpawnPrototypeBox(
    UWorld* World,
    UStaticMesh* Mesh,
    const FVector& Location,
    const FRotator& Rotation,
    const FVector& Scale)
{
    if (!World || !Mesh)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStaticMeshActor* Box = World->SpawnActor<AStaticMeshActor>(Location, Rotation, Params);
    if (!Box)
    {
        return nullptr;
    }

    UStaticMeshComponent* MeshComponent = Box->GetStaticMeshComponent();
    MeshComponent->SetStaticMesh(Mesh);
    MeshComponent->SetMobility(EComponentMobility::Static);
    MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
    Box->SetActorScale3D(Scale);

    return Box;
}
}

ASkateGameMode::ASkateGameMode()
{
    DefaultPawnClass = ASkateCharacter::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PrototypeCube = CubeMesh.Object;
    }
}

void ASkateGameMode::StartPlay()
{
    if (bSpawnPrototypeArena)
    {
        SpawnPrototypeArena();
    }

    Super::StartPlay();
}

void ASkateGameMode::SpawnPrototypeArena()
{
    UWorld* World = GetWorld();
    if (!World || !PrototypeCube)
    {
        return;
    }

    // 50m x 50m ground slab.
    SpawnPrototypeBox(
        World,
        PrototypeCube,
        FVector(0.0f, 0.0f, -10.0f),
        FRotator::ZeroRotator,
        FVector(25.0f, 25.0f, 0.10f));

    // Low ledge for early ollie / collision testing.
    SpawnPrototypeBox(
        World,
        PrototypeCube,
        FVector(700.0f, 300.0f, 25.0f),
        FRotator::ZeroRotator,
        FVector(3.0f, 0.65f, 0.25f));

    // Two banks to validate transitions and board stability.
    SpawnPrototypeBox(
        World,
        PrototypeCube,
        FVector(1150.0f, -350.0f, 55.0f),
        FRotator(10.0f, 0.0f, 0.0f),
        FVector(4.0f, 2.5f, 0.15f));

    SpawnPrototypeBox(
        World,
        PrototypeCube,
        FVector(-900.0f, 500.0f, 40.0f),
        FRotator(-7.0f, 25.0f, 0.0f),
        FVector(3.2f, 2.0f, 0.15f));
}
