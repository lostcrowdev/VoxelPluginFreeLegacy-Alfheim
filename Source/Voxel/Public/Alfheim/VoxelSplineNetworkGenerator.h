#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DataTable.h"
#include "ProceduralMeshComponent.h"
#include "Alfheim/VoxelDelaunayVoronoiUtils.h"
#include "Alfheim/VoxelUnderwaterNetworkComponent.h"
#include "VoxelSplineNetworkGenerator.generated.h"

class AVoxelWorld;

USTRUCT(BlueprintType)
struct FVoxelBiomeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FString BiomeName = TEXT("Forest");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FName PCGTag = TEXT("PCG_Biome_Forest");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FLinearColor DebugColor = FLinearColor(0.13f, 0.55f, 0.13f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	TArray<FName> CanBorderBiomes;
};

USTRUCT(BlueprintType)
struct FVoxelLocationRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FString DisplayName = TEXT("My Location");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FName PCGTag = TEXT("PCG_Loc_MyLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FName CategoryTag = TEXT("PCG_City");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	TArray<FName> AllowedBiomes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", meta = (ClampMin = 100.f))
	float CentreRadius = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", meta = (ClampMin = 0.f))
	float MinSpacingFromOthers = 80000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FLinearColor DebugColor = FLinearColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	bool bConnectsToRoadNetwork = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
		meta = (ToolTip = "Named harbour. Dock circle placed at sea lane endpoint. Sea lane connects shorelines."))
	bool bConnectsToSeaLane = false;
};

USTRUCT(BlueprintType)
struct FVoxelBiomeCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cell") FVector WorldPosition  = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") TArray<int32> NeighborIndices;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") FName BiomeRowName     = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") FName LocationRowName  = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") bool bOnTerrain        = false;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") int32 IslandIndex      = -1;

	TArray<FVector2D> VoronoiPolygon2D;
};

UCLASS(Blueprintable, BlueprintType, meta = (PrioritizeCategories = "Voxel Network"))
class AVoxelSplineNetworkGenerator : public AActor
{
	GENERATED_BODY()

public:
	AVoxelSplineNetworkGenerator();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Runtime")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Runtime",
		meta = (ClampMin = 0.f, DisplayName = "Generation Delay (seconds)"))
	float GenerationDelay = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (DisplayName = "Auto-Size From Voxel World"))
	bool bAutoSizeFromVoxelWorld = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network")
	bool bForceLOD0CollisionBeforeGeneration = false;

	/** Restore the VoxelWorld's original LOD settings once generation finishes (recommended — LOD0-everywhere is expensive to leave on permanently). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "bForceLOD0CollisionBeforeGeneration"))
	bool bRestoreLODAfterGeneration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "!bAutoSizeFromVoxelWorld",
		        ToolTip = "Only used while 'Auto-Size From Voxel World' is OFF. While it's ON, this value is completely ignored — editing it will have no visible effect."))
	float NetworkWidth = 2000000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "bAutoSizeFromVoxelWorld", ClampMin = 1.0f, ClampMax = 3.0f))
	float AutoSizePadding = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "bAutoSizeFromVoxelWorld", ClampMin = 10000.f,
		        DisplayName = "Auto-Size Max Plausible Component Extent",
		        ToolTip = "Any VoxelWorld primitive component reporting a bounds extent bigger than this (cm) is treated as a plugin-internal placeholder/streaming-volume, not real generated terrain, and is skipped when measuring size. Raise this only if your actual voxel world is genuinely larger than the default."))
	float AutoSizeMaxPlausibleComponentExtent = 5000000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "bAutoSizeFromVoxelWorld"))
	TArray<FString> AutoSizeExcludedComponentClassSubstrings = { TEXT("LineBatch") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "bAutoSizeFromVoxelWorld", ClampMin = 4, ClampMax = 64,
		        DisplayName = "Auto-Size Probe Directions",
		        ToolTip = "Number of outward rays used to empirically measure the voxel terrain's extent. More = more accurate for irregular/sparse coastlines, but slower."))
	int32 AutoSizeProbeDirections = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "bAutoSizeFromVoxelWorld", ClampMin = 100.f,
		        DisplayName = "Auto-Size Probe Step",
		        ToolTip = "Distance between probe samples along each ray, in cm. Keep this small — real islands can be a small fraction of the network width, and a coarse step can jump straight over one."))
	float AutoSizeProbeStep = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (EditCondition = "bAutoSizeFromVoxelWorld", ClampMin = 1,
		        DisplayName = "Auto-Size Max Probe Steps",
		        ToolTip = "Safety cap on probe steps per ray. Max probe radius = AutoSizeProbeStep * this value. Every ray always runs the full step count (no early-out on misses, since Voxel Plugin collision-streaming gaps aren't real terrain edges) — so cost = AutoSizeProbeDirections * this value line traces, once per generation."))
	int32 AutoSizeMaxProbeSteps = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network")
	int32 CellCount = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (ClampMin = 0.f, ClampMax = 1.f))
	float JitterAmount = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network")
	int32 Seed = 42;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Biomes")
	TObjectPtr<UDataTable> BiomeDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Locations")
	TObjectPtr<UDataTable> LocationDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Locations",
		meta = (DisplayName = "Force Place If Failed"))
	bool bForcePlaceLocations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Locations",
		meta = (DisplayName = "Stamp Biome Circle On Force-Place",
		        EditCondition = "bForcePlaceLocations"))
	bool bForcePlaceBiomeCircle = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain")
	TObjectPtr<AVoxelWorld> VoxelWorld = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain")
	float TraceStartZ = 200000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain")
	float TraceEndZ = -200000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain",
		meta = (DisplayName = "Min Spawn Height"))
	float MinSpawnHeight = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Roads")
	int32 MaxRoadPathHops = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Roads",
		meta = (ClampMin = 0, ClampMax = 16))
	int32 RoadIntermediateSamples = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Roads")
	float RoadZOffset = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Roads",
		meta = (DisplayName = "Min Road Height"))
	float MinRoadHeight = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Roads",
		meta = (DisplayName = "Road Above-Terrain Lift", ClampMin = 0.f))
	float RoadAboveTerrainLift = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Roads",
		meta = (DisplayName = "Road Lateral Jitter", ClampMin = 0.f))
	float RoadLateralJitter = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Roads",
		meta = (DisplayName = "Road Path Subdivisions", ClampMin = 0, ClampMax = 8))
	int32 RoadPathSubdivisions = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Sea Lanes")
	bool bGenerateSeaLanes = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Sea Lanes",
		meta = (DisplayName = "All Sea Lanes Have Docks"))
	bool bAllSeaLanesHaveDocks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Sea Lanes",
		meta = (DisplayName = "Generic Dock Radius", ClampMin = 100.f,
		        EditCondition = "bAllSeaLanesHaveDocks"))
	float GenericDockRadius = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Sea Lanes",
		meta = (DisplayName = "Sea Lane Z Height"))
	float SeaLaneZHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Sea Lanes",
		meta = (DisplayName = "Sea Lane Z Offset"))
	float SeaLaneZOffset = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Sea Lanes",
		meta = (ClampMin = 0, ClampMax = 64))
	int32 SeaLaneIntermediateSamples = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Coast")
	bool bGenerateCoastSplines = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Coast",
		meta = (ClampMin = 16, ClampMax = 512))
	int32 CoastSampleResolution = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Coast",
		meta = (DisplayName = "Coast Height Threshold"))
	float CoastHeightThreshold = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Coast")
	float CoastZOffset = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Coast",
		meta = (ClampMin = 3))
	int32 CoastMinLoopSegments = 6;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (DisplayName = "Generate Underwater Network"))
	bool bGenerateUnderwaterNetwork = false;

	/** DataTable using FVoxelUnderwaterBiomeRow or FVoxelBiomeRow as its row struct. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (EditCondition = "bGenerateUnderwaterNetwork"))
	TObjectPtr<UDataTable> UnderwaterBiomeDataTable = nullptr;

	/** DataTable using FVoxelUnderwaterLocationRow or FVoxelLocationRow as its row struct. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (EditCondition = "bGenerateUnderwaterNetwork"))
	TObjectPtr<UDataTable> UnderwaterLocationDataTable = nullptr;

	/** Number of Voronoi seed cells for the underwater network. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (EditCondition = "bGenerateUnderwaterNetwork", ClampMin = 4))
	int32 UnderwaterCellCount = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (EditCondition = "bGenerateUnderwaterNetwork"))
	int32 UnderwaterSeed = 142;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (EditCondition = "bGenerateUnderwaterNetwork", ClampMin = 0.f, ClampMax = 1.f))
	float UnderwaterJitterAmount = 0.6f;

	/**
	 * Cells whose traced seabed Z is below this value are excluded (too deep).
	 * Default of -1000000 effectively includes all detectable seabed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (EditCondition = "bGenerateUnderwaterNetwork", DisplayName = "Underwater Floor Z"))
	float UnderwaterFloorZ = -1000000.f;

	/** Force-place underwater locations that cannot find a valid cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Underwater",
		meta = (EditCondition = "bGenerateUnderwaterNetwork",
		        DisplayName = "Force Place Underwater Locations If Failed"))
	bool bUnderwaterForcePlaceLocations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug")
	float DebugDuration = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug")
	float DebugZLift = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Cell Fill")
	bool bShowCellFill = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Cell Fill",
		meta = (ClampMin = 0.05f, ClampMax = 1.f))
	float CellFillBrightness = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Cell Fill")
	TObjectPtr<UMaterialInterface> CellFillMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Edges")
	bool bShowEdges = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Edges")
	FColor EdgeColor = FColor(80, 80, 80);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Edges")
	float EdgeThickness = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Labels")
	bool bShowLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Labels")
	bool bShowBiomeLabelOnAllCells = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Labels")
	float LabelZOffset = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Labels")
	float LabelWorldSize = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Labels",
		meta = (DisplayName = "Billboard Labels (Runtime)"))
	bool bBillboardLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Roads")
	bool bShowRoads = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Roads")
	FColor RoadDebugColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Roads")
	float RoadDebugThickness = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Docks")
	bool bShowDocks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Docks")
	FColor DockDebugColor = FColor(0, 220, 180);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Docks")
	float DockDebugThickness = 22.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Sea Lanes")
	bool bShowSeaLanes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Sea Lanes")
	FColor SeaLaneDebugColor = FColor(0, 140, 255);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Sea Lanes")
	float SeaLaneDebugThickness = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Coast")
	bool bShowCoast = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Coast")
	FColor CoastDebugColor = FColor(0, 180, 255);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Coast")
	float CoastDebugThickness = 24.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Underwater")
	bool bShowUnderwaterEdges = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Underwater")
	FColor UnderwaterEdgeColor = FColor(0, 80, 180);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Underwater")
	float UnderwaterEdgeThickness = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Underwater")
	bool bShowUnderwaterLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Underwater")
	bool bShowUnderwaterBiomeLabelOnAllCells = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Example Data",
		meta = (DisplayName = "Use Example Data If Missing"))
	bool bUseExampleDataIfMissing = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel Network")
	void GenerateNetwork();

	UFUNCTION(BlueprintCallable, Category = "Voxel Network")
	void GenerateNetworkDelayed(float DelaySeconds = -1.f);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel Network")
	void ClearNetwork();

	UFUNCTION(BlueprintCallable, Category = "Voxel Network")
	void ClearNetworkRuntime();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel Network|Debug")
	void RefreshDebugDraw();
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Network|Terrain")
	float ProbeTerrainExtent(bool bLandOnly = false) const;

	// Surface getters
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<FVoxelBiomeCell>&   GetCells()          const { return Cells; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetBiomeSplines()   const { return BiomeSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetCentreSplines()  const { return CentreSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetDockSplines()    const { return DockSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetRoadSplines()    const { return RoadSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetSeaLaneSplines() const { return SeaLaneSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetCoastSplines()   const { return CoastSplines; }

	// Underwater getters
	UFUNCTION(BlueprintPure, Category = "Voxel Network")
	const TArray<FVoxelUnderwaterCell>& GetUnderwaterCells() const { return UnderwaterComponent->GetCells(); }
	UFUNCTION(BlueprintPure, Category = "Voxel Network")
	const TArray<USplineComponent*>& GetUnderwaterBiomeSplines() const { return UnderwaterComponent->GetBiomeSplines(); }
	UFUNCTION(BlueprintPure, Category = "Voxel Network")
	const TArray<USplineComponent*>& GetUnderwaterCentreSplines() const { return UnderwaterComponent->GetCentreSplines(); }
	UFUNCTION(BlueprintPure, Category = "Voxel Network")
	UVoxelUnderwaterNetworkComponent* GetUnderwaterNetworkComponent() const { return UnderwaterComponent; }

	bool TraceGroundAt(float X, float Y, FVector& OutHit) const;
	FBox2D GetNetworkBounds2D() const;
	USplineComponent* MakeSplineComponent(const FString& UniqueName);
	static FString SanitiseName(const FString& In);
	float GetComputedNetworkWidth() const { return ComputedNetworkWidth; }
	FVector2D GetComputedNetworkOrigin() const { return ComputedNetworkOrigin; }
	const TArray<float>& GetCachedHeightGrid() const { return CachedHeightGrid; }
	int32 GetCachedGridRes() const { return CachedGridRes; }
	static constexpr float GRID_MISS = -1e9f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY() TObjectPtr<UVoxelUnderwaterNetworkComponent> UnderwaterComponent;
	UPROPERTY() TArray<FVoxelBiomeCell>              Cells;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> BiomeSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> CentreSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> DockSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> RoadSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> SeaLaneSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> CoastSplines;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent>     FillMeshComponent;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextRenderComponent>> LabelComponents;

	TArray<FName>             BiomeRowNames;
	TArray<FVoxelBiomeRow>    BiomeRows;
	TArray<FName>             LocationRowNames;
	TArray<FVoxelLocationRow> LocationRows;
	TArray<float> CachedHeightGrid;
	TArray<FName> CachedBiomeGrid;
	int32         CachedGridRes = 0;
	float         ComputedNetworkWidth = 2000000.f;
	FVector2D     ComputedNetworkOrigin = FVector2D::ZeroVector;
	TMap<int32, int32>    IslandDockCellMap;
	TSet<int32>           GenericDockCellSet;
	TMap<int32, FVector2D> DockShorelinePointMap;
	TSet<int32> ForcePlacedBiomeCells;
	TArray<FVoxelDelaunayVoronoiUtils::FDelaunayTriangle> CachedTriangles;
	FTimerHandle GenerateDelayHandle;
	
	void RunGenerationPipeline();
	void LoadDataTables();
	void LoadExampleBiomeRows(TArray<FName>& OutNames, TArray<FVoxelBiomeRow>& OutRows) const;
	void LoadExampleLocationRows(TArray<FName>& OutNames, TArray<FVoxelLocationRow>& OutRows) const;
	void GenerateCellPoints();
	void BuildDelaunayGraph();
	void ComputeVoronoiPolygons();
	void AssignBiomes();
	void TraceTerrainHeights();
	void ComputeIslandIndices();
	void AssignLocations();
	void GatherDockCells();
	void SampleHeightGrid();
	void RasterizeBiomeGrid();
	void BuildCellSplines();
	void BuildCentreSplines();
	void BuildDockSplines();   
	void BuildRoadSplines();
	void BuildSeaLaneSplines(); 
	void BuildCoastlineSplines();
	void ClearLabelComponents();
	void BuildLabelComponents();
	void BuildDockLabelComponents();
	void RebuildFillMesh();
	void DrawDebugEdges()           const;
	void DrawDebugLocationMarkers() const;
	void DrawDebugDocks()           const;
	void DrawDebugRoads()           const;
	void DrawDebugSeaLanes()        const;
	void DrawDebugCoast()           const;
	
	TArray<int32> BFSPath(int32 Start, int32 End) const;
	void          CreateRoadSpline(const TArray<int32>& Path, const FString& NameA, const FString& NameB);
	float         DockRadiusForCell(int32 CellIndex) const;
	bool          IsRegisteredDockCell(int32 Ci) const;
	int32         FindNearestReachableCoastalCell(int32 FromCellIdx) const;
	void          CreateWaterCrossingConnection(int32 FromCell, int32 ToCell);
	void          StampEmergencyDockAtCell(int32 DockCi); 
	
	FVector2D FindShorelinePoint(int32 CoastalCellIdx) const;
	
	float        ComputeNetworkWidth() const;
	FVector2D    ComputeNetworkOrigin() const;
	AVoxelWorld* ResolveVoxelWorld()   const;
	float        SafeRoadTerrainZ(float X, float Y, float FallbackZ) const;
	
	bool ProbeTerrainRadius(const FVector2D& Origin, float& OutMaxRadius, bool bLandOnly) const;
	bool ComputeFilteredActorBounds2D(AActor* Actor, FBox2D& OutBounds2D) const;
	
	void ForceCollisionAcrossVoxelWorld(AVoxelWorld* VW);
	void RestoreVoxelWorldLOD(AVoxelWorld* VW);

	int32 SavedVoxelWorldMaxLOD      = 0;
	int32 SavedVoxelWorldMinLOD      = 0;
	bool  bSavedVoxelWorldConstantLOD = false;
	bool  bVoxelWorldLODForced        = false;
	
	FColor GetBiomeColor(FName RowName, float Brightness = 1.f) const;
	FColor GetLocationColor(FName RowName) const;
};