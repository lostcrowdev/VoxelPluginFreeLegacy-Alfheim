#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DataTable.h"
#include "Alfheim/VoxelDelaunayVoronoiUtils.h"
#include "VoxelUnderwaterNetworkComponent.generated.h"

class AVoxelSplineNetworkGenerator;

USTRUCT(BlueprintType)
struct FVoxelUnderwaterBiomeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Biome")
	FString BiomeName = TEXT("Kelp Forest");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Biome")
	FName PCGTag = TEXT("PCG_UW_Biome_KelpForest");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Biome")
	FLinearColor DebugColor = FLinearColor(0.0f, 0.3f, 0.55f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Biome", meta = (ClampMin = 0.f))
	float PreferredMinDepth = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Biome", meta = (ClampMin = 0.f))
	float PreferredMaxDepth = 500000.f;

	/** Which biomes this biome is allowed to border. Empty = any. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Biome")
	TArray<FName> CanBorderBiomes;
};

USTRUCT(BlueprintType)
struct FVoxelUnderwaterLocationRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location")
	FString DisplayName = TEXT("Sunken Temple");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location")
	FName PCGTag = TEXT("PCG_UW_Loc_SunkenTemple");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location")
	FName CategoryTag = TEXT("PCG_UW_Ruin");

	/** Biomes this location is allowed to be placed in. Empty = any. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location")
	TArray<FName> AllowedBiomes;

	/** Radius of the seabed centre circle drawn around this location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location", meta = (ClampMin = 100.f))
	float CentreRadius = 8000.f;

	/** Minimum world-space distance from any other placed underwater location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location", meta = (ClampMin = 0.f))
	float MinSpacingFromOthers = 60000.f;

	/** Minimum depth below CoastHeightThreshold (world units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location", meta = (ClampMin = 0.f))
	float MinDepth = 0.f;

	/** Maximum depth below CoastHeightThreshold (world units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location", meta = (ClampMin = 0.f))
	float MaxDepth = 999999.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underwater Location")
	FLinearColor DebugColor = FLinearColor(0.0f, 0.6f, 1.0f);
};

USTRUCT(BlueprintType)
struct FVoxelUnderwaterCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Underwater Cell") FVector WorldPosition  = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Underwater Cell") TArray<int32> NeighborIndices;
	UPROPERTY(BlueprintReadOnly, Category = "Underwater Cell") FName BiomeRowName     = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "Underwater Cell") FName LocationRowName  = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "Underwater Cell") bool  bUnderwater = false;
	UPROPERTY(BlueprintReadOnly, Category = "Underwater Cell") float SeabedZ    = 0.f;

	TArray<FVector2D> VoronoiPolygon2D;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UVoxelUnderwaterNetworkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoxelUnderwaterNetworkComponent();

	void RunPipeline();

	void Clear();

	void RefreshDebugDraw();

	UFUNCTION(BlueprintPure, Category = "Voxel Network|Underwater")
	const TArray<FVoxelUnderwaterCell>& GetCells() const { return UnderwaterCells; }

	UFUNCTION(BlueprintPure, Category = "Voxel Network|Underwater")
	const TArray<USplineComponent*>& GetBiomeSplines() const { return UnderwaterBiomeSplines; }

	UFUNCTION(BlueprintPure, Category = "Voxel Network|Underwater")
	const TArray<USplineComponent*>& GetCentreSplines() const { return UnderwaterCentreSplines; }

	const TArray<TObjectPtr<UTextRenderComponent>>& GetLabelComponents() const { return LabelComponents; }

private:
	AVoxelSplineNetworkGenerator* GetGenerator() const;
	
	UPROPERTY() TArray<FVoxelUnderwaterCell>         UnderwaterCells;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> UnderwaterBiomeSplines;
	UPROPERTY() TArray<TObjectPtr<USplineComponent>> UnderwaterCentreSplines;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextRenderComponent>> LabelComponents;

	TArray<FName>                       UnderwaterBiomeRowNames;
	TArray<FVoxelUnderwaterBiomeRow>    UnderwaterBiomeRows;
	TArray<FName>                       UnderwaterLocationRowNames;
	TArray<FVoxelUnderwaterLocationRow> UnderwaterLocationRows;
	TArray<FName>                       CachedUnderwaterBiomeGrid;

	TArray<FVoxelDelaunayVoronoiUtils::FDelaunayTriangle> CachedUnderwaterTriangles;
	
	void LoadUnderwaterDataTables();
	void LoadExampleUnderwaterBiomeRows(TArray<FName>& OutNames, TArray<FVoxelUnderwaterBiomeRow>& OutRows) const;
	void LoadExampleUnderwaterLocationRows(TArray<FName>& OutNames, TArray<FVoxelUnderwaterLocationRow>& OutRows) const;
	void GenerateUnderwaterCellPoints();
	void BuildUnderwaterDelaunayGraph();
	void ComputeUnderwaterVoronoiPolygons();
	void TraceUnderwaterDepths();
	void AssignUnderwaterBiomes();
	void AssignUnderwaterLocations();
	void RasterizeUnderwaterBiomeGrid();
	void BuildUnderwaterCellSplines();
	void BuildUnderwaterCentreSplines();
	
	void ClearLabelComponents();
	void BuildLabelComponents();
	void DrawDebugEdges() const;
};