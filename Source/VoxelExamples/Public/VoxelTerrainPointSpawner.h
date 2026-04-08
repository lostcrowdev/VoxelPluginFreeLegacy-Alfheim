#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelTerrainPointSpawner.generated.h"

class AVoxelWorld;
class UInstancedStaticMeshComponent;

/**
 * Drop this actor into your level to automatically place point-grid cubes
 * onto a pre-generated Voxel World surface.
 *
 * Workflow:
 *   1. Place this actor anywhere in the level.
 *   2. Set VoxelWorld reference (or leave null to auto-find).
 *   3. Tweak GridWidth, TotalPoints, cube settings.
 *   4. Hit "Spawn Points" in Details panel (CallInEditor) or let BeginPlay do it.
 */
UCLASS(Blueprintable, BlueprintType)
class AVoxelTerrainPointSpawner : public AActor
{
	GENERATED_BODY()

public:
	AVoxelTerrainPointSpawner();

	// -----------------------------------------------------------------------
	// Grid Settings
	// -----------------------------------------------------------------------

	/** Total number of cubes to attempt to place. Grid will be sqrt(N) x sqrt(N). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid")
	int32 TotalPoints = 100;

	/** Width (and depth) of the grid area in Unreal units (cm). e.g. 10000 = 100m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid")
	float GridWidth = 10000.f;

	/**
	 * Z position above the known terrain max to start each surface search.
	 * Increase if your terrain has very tall peaks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid|Search")
	float SearchMinVoxelZ = -2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid|Search")
	float SearchMaxVoxelZ = 2000.f;

	/** Number of binary search iterations when finding surface height. More = more precise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid|Search", meta = (ClampMin = 4, ClampMax = 32))
	int32 SurfaceSearchIterations = 20;

	/** Small offset added to the found surface Z in world units, so cubes sit on top cleanly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid|Search")
	float SurfaceZOffset = 50.f;

	// -----------------------------------------------------------------------
	// Cube Appearance
	// -----------------------------------------------------------------------

	/** Scale applied to each spawned cube instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid|Cubes")
	FVector CubeScale = FVector(0.5f);

	// -----------------------------------------------------------------------
	// Runtime
	// -----------------------------------------------------------------------

	/**
	 * Explicit Voxel World reference. If null, the actor will automatically
	 * find the first AVoxelWorld in the level at spawn time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid")
	TObjectPtr<AVoxelWorld> VoxelWorld = nullptr;

	/** If true, SpawnPoints() is called automatically on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Grid")
	bool bSpawnOnBeginPlay = true;

	// -----------------------------------------------------------------------
	// Functions
	// -----------------------------------------------------------------------

	/** Clears existing cubes and re-spawns the full grid. Safe to call multiple times. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Point Grid")
	void SpawnPoints();

	/** Removes all spawned cube instances. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Point Grid")
	void ClearPoints();

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ISMComponent;

	/** Resolve VoxelWorld — uses set reference or finds first one in world. */
	AVoxelWorld* ResolveVoxelWorld() const;

	/**
	 * Binary-searches the voxel SDF along Z to find the surface height at (VoxelX, VoxelY).
	 * Returns the surface Z in VOXEL space, or TNumericLimits<float>::Lowest() on failure.
	 */
	float FindSurfaceVoxelZ(AVoxelWorld* InVoxelWorld, float VoxelX, float VoxelY) const;

	/** Convert a voxel-space position to Unreal world-space. */
	FVector VoxelToWorld(AVoxelWorld* InVoxelWorld, float VoxelX, float VoxelY, float VoxelZ) const;
};