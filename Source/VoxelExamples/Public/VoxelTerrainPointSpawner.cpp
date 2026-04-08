// VoxelTerrainPointSpawner.cpp

#include "VoxelTerrainPointSpawner.h"

// Voxel Plugin (free) includes
#include "VoxelWorld.h"
#include "VoxelGenerators/VoxelGeneratorInstance.h"
#include "VoxelData/VoxelDataIncludes.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AVoxelTerrainPointSpawner::AVoxelTerrainPointSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root scene component so the actor has a transform in the level
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Instanced Static Mesh for zero-overhead cube rendering
	ISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISM_Cubes"));
	ISMComponent->SetupAttachment(Root);
	ISMComponent->SetMobility(EComponentMobility::Static);

	// Load the engine's built-in 1x1x1 cube mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (CubeMeshFinder.Succeeded())
	{
		ISMComponent->SetStaticMesh(CubeMeshFinder.Object);
	}
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AVoxelTerrainPointSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		SpawnPoints();
	}
}

// ---------------------------------------------------------------------------
// Editor property change — optional live-preview while tweaking values
// ---------------------------------------------------------------------------

#if WITH_EDITOR
void AVoxelTerrainPointSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Uncomment the line below if you want the grid to refresh every time
	// you change a property in the Details panel (can be slow on large grids):
	// SpawnPoints();
}
#endif

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AVoxelTerrainPointSpawner::ClearPoints()
{
	if (ISMComponent)
	{
		ISMComponent->ClearInstances();
	}
}

void AVoxelTerrainPointSpawner::SpawnPoints()
{
	ClearPoints();

	AVoxelWorld* World = ResolveVoxelWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelTerrainPointSpawner [%s]: No AVoxelWorld found. "
			     "Assign one via the VoxelWorld property or make sure one exists in the level."),
			*GetName());
		return;
	}

	if (!World->IsCreated())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelTerrainPointSpawner [%s]: AVoxelWorld is not yet created/generated. "
			     "Make sure the voxel world has finished generating before calling SpawnPoints."),
			*GetName());
		return;
	}

	if (TotalPoints <= 0 || GridWidth <= 0.f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelTerrainPointSpawner [%s]: TotalPoints and GridWidth must be > 0."),
			*GetName());
		return;
	}

	// ------------------------------------------------------------------
	// Build an evenly-spaced 2-D grid centred on this actor's XY position
	// ------------------------------------------------------------------

	// Work out grid dimensions (as square as possible)
	const int32 GridDim    = FMath::CeilToInt(FMath::Sqrt((float)TotalPoints)); // cells per axis
	const float CellSpacing = GridWidth / FMath::Max(GridDim - 1, 1);           // UE units between cells
	const float HalfWidth   = GridWidth * 0.5f;

	// Actor position is the grid centre
	const FVector GridCenter = GetActorLocation();

	int32 SpawnedCount = 0;

	for (int32 Row = 0; Row < GridDim && SpawnedCount < TotalPoints; ++Row)
	{
		for (int32 Col = 0; Col < GridDim && SpawnedCount < TotalPoints; ++Col)
		{
			// World-space XY of this grid cell
			const float WorldX = GridCenter.X - HalfWidth + Col * CellSpacing;
			const float WorldY = GridCenter.Y - HalfWidth + Row * CellSpacing;

			// Convert XY to voxel space so we can query the generator
			const float VoxelX = WorldX / World->VoxelSize;
			const float VoxelY = WorldY / World->VoxelSize;

			// Find the surface Z in voxel space
			const float SurfaceVoxelZ = FindSurfaceVoxelZ(World, VoxelX, VoxelY);

			if (SurfaceVoxelZ == TNumericLimits<float>::Lowest())
			{
				// No surface found at this XY (e.g. completely underground or above terrain)
				continue;
			}

			// Convert back to world space and apply a small lift so cubes sit on top
			FVector WorldPos = VoxelToWorld(World, VoxelX, VoxelY, SurfaceVoxelZ);
			WorldPos.Z += SurfaceZOffset;

			// Place the cube instance.
			// Instances use local space relative to the ISM component.
			FTransform InstanceTransform;
			InstanceTransform.SetLocation(WorldPos - GetActorLocation()); // local to root
			InstanceTransform.SetScale3D(CubeScale);

			ISMComponent->AddInstance(InstanceTransform);
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("VoxelTerrainPointSpawner [%s]: Spawned %d / %d cubes."),
		*GetName(), SpawnedCount, TotalPoints);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

AVoxelWorld* AVoxelTerrainPointSpawner::ResolveVoxelWorld() const
{
	if (VoxelWorld)
	{
		return VoxelWorld;
	}

	// Auto-find the first AVoxelWorld in the level
	AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass());
	return Cast<AVoxelWorld>(Found);
}

float AVoxelTerrainPointSpawner::FindSurfaceVoxelZ(AVoxelWorld* InVoxelWorld, float VoxelX, float VoxelY) const
{
	// Since terrain is pre-generated, a line trace is the most reliable approach
	// and avoids any direct generator API dependency.
	const FVector WorldXY = FVector(VoxelX * InVoxelWorld->VoxelSize, VoxelY * InVoxelWorld->VoxelSize, 0.f)
		+ InVoxelWorld->GetActorLocation();

	const FVector TraceStart = FVector(WorldXY.X, WorldXY.Y, SearchMaxVoxelZ * InVoxelWorld->VoxelSize);
	const FVector TraceEnd   = FVector(WorldXY.X, WorldXY.Y, SearchMinVoxelZ * InVoxelWorld->VoxelSize);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		// Return the hit Z back in voxel space
		return (Hit.ImpactPoint.Z - InVoxelWorld->GetActorLocation().Z) / InVoxelWorld->VoxelSize;
	}

	return TNumericLimits<float>::Lowest();
}

FVector AVoxelTerrainPointSpawner::VoxelToWorld(AVoxelWorld* InVoxelWorld, float VoxelX, float VoxelY, float VoxelZ) const
{
	// LocalToGlobal expects FIntVector in this plugin version
	return InVoxelWorld->LocalToGlobal(FIntVector(
		FMath::RoundToInt(VoxelX),
		FMath::RoundToInt(VoxelY),
		FMath::RoundToInt(VoxelZ)
	));
}