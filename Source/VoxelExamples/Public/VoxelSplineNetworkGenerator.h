#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DataTable.h"
#include "ProceduralMeshComponent.h"
#include "VoxelSplineNetworkGenerator.generated.h"

class AVoxelWorld;

// ---------------------------------------------------------------------------
// Biome DataTable row
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Location DataTable row
// ---------------------------------------------------------------------------

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
};

// ---------------------------------------------------------------------------
// Internal cell
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FVoxelBiomeCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cell") FVector WorldPosition = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") TArray<int32> NeighborIndices;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") FName BiomeRowName = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") FName LocationRowName = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "Cell") bool bOnTerrain = false;

	// VoronoiPolygon2D is kept for the fill-mesh debug visualisation only.
	// It is NO LONGER used to build biome splines.
	TArray<FVector2D> VoronoiPolygon2D;
};

// ---------------------------------------------------------------------------
// Actor
// ---------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType, meta = (PrioritizeCategories = "Voxel Network"))
class AVoxelSplineNetworkGenerator : public AActor
{
	GENERATED_BODY()

public:
	AVoxelSplineNetworkGenerator();

	// Runtime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Runtime")
	bool bGenerateOnBeginPlay = true;

	/**
	 * How long to wait (in seconds) before tracing when using
	 * GenerateNetworkDelayed. The voxel world rebuilds collision
	 * asynchronously after a mesh regenerate — this delay ensures traces
	 * hit the new terrain, not the old one. Tune upward for larger worlds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Runtime",
		meta = (ClampMin = 0.f, DisplayName = "Generation Delay (seconds)"))
	float GenerationDelay = 1.0f;

	// Cell Network
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network")
	float NetworkWidth = 2000000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network")
	int32 CellCount = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network",
		meta = (ClampMin = 0.f, ClampMax = 1.f))
	float JitterAmount = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Cell Network")
	int32 Seed = 42;

	// Biomes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Biomes")
	TObjectPtr<UDataTable> BiomeDataTable = nullptr;

	// Locations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Locations")
	TObjectPtr<UDataTable> LocationDataTable = nullptr;

	// Terrain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain")
	TObjectPtr<AVoxelWorld> VoxelWorld = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain")
	float TraceStartZ = 200000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain")
	float TraceEndZ = -200000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Terrain",
		meta = (DisplayName = "Min Spawn Height"))
	float MinSpawnHeight = 0.f;

	// Roads
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

	// -----------------------------------------------------------------------
	// Voxel Network | Coast
	// -----------------------------------------------------------------------

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

	// Debug
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Roads")
	bool bShowRoads = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Roads")
	FColor RoadDebugColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Roads")
	float RoadDebugThickness = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Coast")
	bool bShowCoast = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Coast")
	FColor CoastDebugColor = FColor(0, 180, 255);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Network|Debug|Coast")
	float CoastDebugThickness = 24.f;

	// -----------------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------------

	/**
	 * Full pipeline: clears then regenerates immediately.
	 * Only use this when you know the voxel world's collision is already
	 * up to date (e.g. on first BeginPlay before any seed change).
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel Network")
	void GenerateNetwork();

	/**
	 * Clears immediately, then waits GenerationDelay seconds before running
	 * the full pipeline. Always use this after changing the voxel world seed
	 * so that async collision cooking has time to finish before traces fire.
	 * Passing a DelaySeconds >= 0 overrides the GenerationDelay property
	 * for this one call; passing -1 (the default) uses the property value.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel Network")
	void GenerateNetworkDelayed(float DelaySeconds = -1.f);

	/** Destroys all splines, cells, and cached data. Safe to call at runtime. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel Network")
	void ClearNetwork();

	/** Alias for ClearNetwork — kept for runtime Blueprint compatibility. */
	UFUNCTION(BlueprintCallable, Category = "Voxel Network")
	void ClearNetworkRuntime();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel Network|Debug")
	void RefreshDebugDraw();

	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<FVoxelBiomeCell>&   GetCells()         const { return Cells; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetBiomeSplines()  const { return BiomeSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetCentreSplines() const { return CentreSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetRoadSplines()   const { return RoadSplines; }
	UFUNCTION(BlueprintPure, Category = "Voxel Network") const TArray<USplineComponent*>& GetCoastSplines()  const { return CoastSplines; }

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY() TArray<FVoxelBiomeCell> Cells;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> BiomeSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> CentreSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> RoadSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> CoastSplines;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> FillMeshComponent;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextRenderComponent>> LabelComponents;

	TArray<FName>             BiomeRowNames;
	TArray<FVoxelBiomeRow>    BiomeRows;
	TArray<FName>             LocationRowNames;
	TArray<FVoxelLocationRow> LocationRows;

	TArray<float> CachedHeightGrid;
	TArray<FName> CachedBiomeGrid;
	int32         CachedGridRes = 0;

	struct FDelaunayTriangle { int32 A, B, C; };
	struct FDelaunayEdge
	{
		int32 A, B;
		bool operator==(const FDelaunayEdge& O) const { return (A==O.A&&B==O.B)||(A==O.B&&B==O.A); }
	};
	TArray<FDelaunayTriangle> CachedTriangles;

	// Timer used by GenerateNetworkDelayed
	FTimerHandle GenerateDelayHandle;

	// Pipeline — called directly by GenerateNetwork, or via timer by GenerateNetworkDelayed
	void RunGenerationPipeline();

	void LoadDataTables();
	void GenerateCellPoints();
	void BuildDelaunayGraph();
	void ComputeVoronoiPolygons();
	void AssignBiomes();
	void TraceTerrainHeights();
	void AssignLocations();
	void SampleHeightGrid();
	void RasterizeBiomeGrid();
	void BuildCellSplines();
	void BuildCentreSplines();
	void BuildRoadSplines();
	void BuildCoastlineSplines();

	// Debug
	void ClearLabelComponents();
	void BuildLabelComponents();
	void RebuildFillMesh();
	void DrawDebugEdges()           const;
	void DrawDebugLocationMarkers() const;
	void DrawDebugRoads()           const;
	void DrawDebugCoast()           const;

	// Roads
	TArray<int32> BFSPath(int32 Start, int32 End) const;
	void CreateRoadSpline(const TArray<int32>& Path, const FString& NameA, const FString& NameB);

	// Terrain
	AVoxelWorld* ResolveVoxelWorld() const;
	bool TraceGroundAt(float X, float Y, FVector& OutHit) const;

	// Delaunay / Voronoi
	TArray<FDelaunayTriangle> BowyerWatson() const;
	void GetCircumcircle(const FVector2D& A, const FVector2D& B, const FVector2D& C,
	                     FVector2D& OutCenter, float& OutRadiusSq) const;
	TArray<FVector2D> ComputeVoronoiPolygon2D(int32 CellIdx) const;
	FBox2D GetNetworkBounds2D() const;

	TArray<TArray<FVector2D>> ExtractMarchingSquaresLoops(
		TFunctionRef<bool(int32 Row, int32 Col)> InsideFn,
		TFunctionRef<float(int32 Row, int32 Col)> ValueFn,
		float Threshold) const;

	// Helpers
	FColor  GetBiomeColor(FName RowName, float Brightness = 1.f) const;
	FColor  GetLocationColor(FName RowName) const;
	USplineComponent* MakeSplineComponent(const FString& UniqueName);
	static FString SanitiseName(const FString& In);

	static constexpr float GRID_MISS = -1e9f;
};