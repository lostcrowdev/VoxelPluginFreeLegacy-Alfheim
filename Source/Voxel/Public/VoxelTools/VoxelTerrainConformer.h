// VoxelTerrainConformer.h
// Lost Crow Dev - https://github.com/lostcrowdev/VoxelPluginFreeLegacy-Alfheim
// Custom tool to shape voxel terrain around tagged actors.
// Place in level, set VoxelWorld, PCG volumes, and tag actors to affect.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelWorld.h"
#include "VoxelTools/VoxelBlueprintLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "VoxelTerrainConformer.generated.h"

class APCGVolume;
class UPCGComponent;

UCLASS(BlueprintType, Blueprintable)
class VOXEL_API AVoxelTerrainConformer : public AActor
{
	GENERATED_BODY()
	
public:	
	AVoxelTerrainConformer();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	/** Voxel world to edit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Conformer")
	AVoxelWorld* VoxelWorld;
	
	// Spline flattening
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Spline Terraforming")
	bool bEnableSplineTerraforming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Spline Terraforming", meta = (EditCondition = "bEnableSplineTerraforming"))
	FName FlattenSplineTag = FName("FlattenArea");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Spline Terraforming", meta = (EditCondition = "bEnableSplineTerraforming", ClampMin = "-10000.0", ClampMax = "10000.0"))
	float FlattenHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Spline Terraforming", meta = (EditCondition = "bEnableSplineTerraforming", ClampMin = "0.0", ClampMax = "1.0"))
	float FlattenStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Spline Terraforming", meta = (EditCondition = "bEnableSplineTerraforming", ClampMin = "0.0", ClampMax = "2000.0"))
	float FlattenEdgeFalloff = 200.0f;
	
	// Procedural generation volumes (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. PCG Generation")
	bool bGeneratePCG = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. PCG Generation", meta = (EditCondition = "bGeneratePCG"))
	TArray<class APCGVolume*> PCGVolumes;
	
	// Detail conforming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming")
	bool bEnableDetailConforming = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming", meta = (EditCondition = "bEnableDetailConforming"))
	FName EnvironmentActorTag = "Environment";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming", meta = (EditCondition = "bEnableDetailConforming"))
	FName ConformPointTag = "ConformPoint";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming", meta = (EditCondition = "bEnableDetailConforming"))
	bool bRequireConformPoints = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming", meta = (EditCondition = "bEnableDetailConforming", ClampMin = "-500.0", ClampMax = "500.0"))
	float TerrainOffset = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming", meta = (EditCondition = "bEnableDetailConforming", ClampMin = "0.0", ClampMax = "1000.0"))
	float EditRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming", meta = (EditCondition = "bEnableDetailConforming", ClampMin = "5.0", ClampMax = "45.0"))
	float SlopeAngle = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming", meta = (EditCondition = "bEnableDetailConforming", ClampMin = "500.0", ClampMax = "5000.0"))
	float SlopeDistance = 2000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming|Advanced", meta = (EditCondition = "bEnableDetailConforming"))
	bool bUseTwoPassSystem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming|Advanced", meta = (EditCondition = "bEnableDetailConforming", ClampMin = "500.0", ClampMax = "10000.0"))
	float LargeSlopeRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming|Advanced", meta = (EditCondition = "bEnableDetailConforming", ClampMin = "0.0", ClampMax = "1.0"))
	float LargeSlopeStrength = 0.3f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Detail Conforming|Advanced", meta = (EditCondition = "bEnableDetailConforming", ClampMin = "1", ClampMax = "100"))
	int32 MaxFillDepthVoxels = 20;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Conformer|Behavior")
	bool bAutoProcessOnBeginPlay = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Conformer|Behavior")
	bool bContinuousUpdate = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Conformer|Behavior", meta = (EditCondition = "bContinuousUpdate", ClampMin = "0.1"))
	float UpdateInterval = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Conformer|Advanced")
	float BoundsPadding = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Conformer|Debug", meta = (ClampMin = "0.0"))
	float DebugLifetime = 0.0f;

	// Runtime
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer")
	void ProcessAllTaggedActors();
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer")
	void ProcessSingleActor(AActor* Actor);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer")
	void ProcessActorBatch(const TArray<AActor*>& Actors);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer")
	TArray<AActor*> FindEnvironmentActors();
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer")
	FVoxelIntBox GetActorVoxelBounds(AActor* Actor);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer")
	TArray<FVector> GetConformPointsForActor(AActor* Actor);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer")
	void ResetContinuousUpdateTracking();
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer|Debug")
	void DebugPrintSystemStatus();
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Conformer|Debug")
	void DebugCheckTerrainNearActor(AActor* Actor);

protected:
	// Internal
	UFUNCTION()
	void OnVoxelWorldLoaded();
	
	void ExecuteTerrainPipeline();
	void ProcessSplineTerraforming();
	void ProcessPCGGeneration();
	void ProcessDetailConforming();
	
	UFUNCTION()
	void OnPCGVolumeCompleted(UPCGComponent* PCGComponent);
	bool AreAllPCGVolumesComplete() const;
	void ConformTerrainAtLocation(const FVector& Location, const FVoxelIntBox& Bounds);
	void ApplyLargeScaleSlopes(const TArray<FVector>& AllConformPoints);
	void ApplyPreciseFloorAdjustment(const FVector& Location, const FVoxelIntBox& Bounds);
	TArray<class USplineComponent*> FindFlattenSplines();
	void FlattenTerrainInSpline(USplineComponent* Spline);
	bool IsPointInSpline(const FVector2D& Point, USplineComponent* Spline);
	float GetExistingVoxelValue(const FIntVector& VoxelPos);
	bool HasActorMoved(AActor* Actor);
	void UpdateActorPosition(AActor* Actor);
	FIntVector FindNearestTerrainSurface(const FIntVector& StartVoxel, int32 SearchRadius);
	void DrawDebugVisualization(
		AActor* Actor,
		const FVector& PointLocation,
		const FVector& SurfaceLocation,
		const FVoxelIntBox& EditBounds,
		const FBox& ActorWorldBounds,
		int32 PointIndex,
		int32 TotalPoints);

private:
	FTimerHandle ContinuousUpdateTimer;
	TMap<TWeakObjectPtr<AActor>, FVector> LastKnownPositions;
	TArray<FVoxelIntBox> AllModifiedBounds;
	TSet<TWeakObjectPtr<UPCGComponent>> CompletedPCGComponents;
	int32 TotalPCGComponentsToWait = 0;

	enum class EPipelineStep : uint8
	{
		Idle,
		SplineTerraforming,
		PCGGeneration,
		DetailConforming,
		Complete
	};
	
	EPipelineStep CurrentPipelineStep = EPipelineStep::Idle;
};
