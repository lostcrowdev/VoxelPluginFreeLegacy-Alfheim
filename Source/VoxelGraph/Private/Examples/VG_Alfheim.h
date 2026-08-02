#pragma once

#include "CoreMinimal.h"
#include "VoxelGeneratedWorldGeneratorsIncludes.h"
#include "VG_Alfheim.generated.h"

// One stepped shelf chain. Set StepCount above 1 to stack more shelves further out.
// Used for both the mountain terraces (stepping up) and ocean terraces (stepping down) -
// same struct, just pointed in opposite directions.
USTRUCT(BlueprintType)
struct FVoxelTerraceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terracing", meta=(DisplayName="Step Count", ClampMin="0", ClampMax="12", ToolTip="Raise this to add more shelves further out. Set to 0 to turn the whole chain off."))
	int32 StepCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terracing", meta=(DisplayName="Base Height", ToolTip="This is the height of the first shelf. Raise or lower it to move that shelf up or down."))
	float BaseHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terracing", meta=(DisplayName="Step Increment", ClampMin="1.0", ToolTip="How far apart each shelf sits from the last. Raise this for bigger gaps between shelves."))
	float HeightIncrement = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terracing", meta=(DisplayName="Increment Growth", ClampMin="0.1", ClampMax="4.0", ToolTip="Raise above 1.0 to make later shelves sit further apart than earlier ones. Leave at 1.0 for even spacing."))
	float IncrementGrowth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terracing", meta=(DisplayName="Shelf Width", ClampMin="1.0", ToolTip="How wide each flat shelf is. Raise this for a wider flat area, lower it for a narrower ledge."))
	float ShelfWidth = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terracing", meta=(DisplayName="Shelf Width Growth", ClampMin="0.1", ClampMax="4.0", ToolTip="Raise above 1.0 to make later shelves wider than earlier ones. Leave at 1.0 for even width."))
	float ShelfWidthGrowth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terracing", meta=(DisplayName="Blend Sharpness", ClampMin="0.1", ClampMax="10.0", ToolTip="Raise this to make the shelf snap flatter and more table-top like. Lower it for a softer, more gradual blend."))
	float BlendSharpness = 2.0f;
};

UCLASS(Blueprintable)
class UVG_Alfheim : public UVoxelGraphGeneratorHelper
{
	GENERATED_BODY()

public:
	// --------------------------------------------------------------------------------
	// Terrain Shape - the base noise everything else builds on top of.
	// --------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Seed"))
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Height Scale", ClampMin="1.0", ClampMax="10000.0", ToolTip="Raise this for taller mountains and deeper natural dips. Lower it for flatter terrain overall."))
	float HeightScale = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Noise Frequency", ClampMin="0.0001", ClampMax="1.0", ToolTip="Lower this to spread terrain features out wider (bigger mountains, bigger islands, bigger everything). Raise it for tighter, busier terrain."))
	float NoiseFrequency = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Noise Octaves", ClampMin="1", ClampMax="10", ToolTip="Raise this for more fine detail layered into the terrain. Lower it for smoother, simpler shapes."))
	int32 NoiseOctaves = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Noise Gain", ClampMin="0.0", ClampMax="1.0", ToolTip="Raise this for rougher, noisier detail. Lower it for smoother terrain."))
	float NoiseGain = 0.59664f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Noise Lacunarity", ClampMin="1.0", ClampMax="4.0", ToolTip="Raise this to make each detail layer jump in frequency more sharply. Usually fine left alone."))
	float NoiseLacunarity = 1.660165f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Mountain Height Exponent", ClampMin="0.2", ClampMax="5.0", ToolTip="Raise this to make tall mountains rarer (more low hills instead). Lower it below 1.0 to make full-height mountains more common. Doesn't change the land/ocean ratio, only how land is shaped."))
	float MountainHeightExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Ocean Depth Exponent", ClampMin="0.2", ClampMax="5.0", ToolTip="Lower this below 1.0 to make deep ocean more common. Raise it to keep most of the sea shallow. Doesn't change the land/ocean ratio, only how the ocean is shaped."))
	float OceanDepthExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Shape", meta=(DisplayName="Land / Ocean Balance", ToolTip="This is the one that actually shifts how much of the map is land vs ocean. Raise it for more land, lower it for more ocean."))
	float LandOceanBalance = 0.0f;

	// --------------------------------------------------------------------------------
	// Island - carves everything past a radius away, so the terrain reads as one landmass.
	// --------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island", meta=(DisplayName="Enable Island Mode"))
	bool bEnableIslandMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island", meta=(DisplayName="Island Radius", ClampMin="100.0", ClampMax="10000.0", EditCondition="bEnableIslandMode", ToolTip="Raise this to make the island bigger. This is measured from world center (0,0)."))
	float IslandRadius = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island", meta=(DisplayName="Island Falloff Distance", ClampMin="50.0", ClampMax="5000.0", EditCondition="bEnableIslandMode", ToolTip="Raise this for a longer, more gradual shoreline transition. Lower it for a sharper edge to the island."))
	float IslandFalloffDistance = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island", meta=(DisplayName="Island Falloff Strength", ClampMin="0.1", ClampMax="5.0", EditCondition="bEnableIslandMode", ToolTip="Raise this for a sharper cliff-like drop at the island edge. Lower it toward 1.0 for a straight, linear falloff."))
	float IslandFalloffStrength = 1.5f;

	// --------------------------------------------------------------------------------
	// Beach - flattens a band of terrain around your waterline so the water sits naturally.
	// --------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach", meta=(DisplayName="Enable Beach"))
	bool bEnableBeachPlateau = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach", meta=(DisplayName="Beach Height", ToolTip="Set this to your waterline height. Raise it for a higher beach, lower it for a lower one."))
	float BeachHeight = -3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach", meta=(DisplayName="Beach Width", ClampMin="1.0", ClampMax="500.0", ToolTip="How far above and below Beach Height the flattening reaches. Lower this for a narrower, tighter beach band. Watch that it doesn't reach far enough down to swallow your shallow ocean shelf."))
	float BeachWidth = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach", meta=(DisplayName="Beach Blend Sharpness", ClampMin="0.1", ClampMax="10.0", ToolTip="Raise this for a flatter, more shelf-like beach. Lower it toward 0.1 for a softer, more gradual slope."))
	float BeachBlendSharpness = 1.0f;

	// --------------------------------------------------------------------------------
	// Mountain Terraces - only ever touches terrain above each shelf, never the ocean.
	// --------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountain Terraces", meta=(DisplayName="Mountain Terraces"))
	FVoxelTerraceSettings MountainTerraces;

	// --------------------------------------------------------------------------------
	// Ocean Terraces - only ever touches terrain below each shelf, never dry land.
	// --------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean Terraces", meta=(DisplayName="Ocean Terraces"))
	FVoxelTerraceSettings OceanTerraces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean Terraces", meta=(DisplayName="Enable Ocean Floor", ToolTip="This guarantees a minimum ocean depth even where the shelves above don't reach that far. Turn it off if you want the ocean floor to depend on the shelves alone."))
	bool bEnableOceanFloor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean Terraces", meta=(DisplayName="Ocean Floor Depth", EditCondition="bEnableOceanFloor", ToolTip="Lower this for a deeper guaranteed ocean floor. Raise it toward 0 for a shallower one."))
	float OceanFloorDepth = -800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean Terraces", meta=(DisplayName="Ocean Floor Ramp Distance", ClampMin="1.0", EditCondition="bEnableOceanFloor", ToolTip="Raise this for a more gradual slope down to the ocean floor. Lower it for a sharper drop-off right past the shallow shelf."))
	float OceanFloorRampDistance = 100.0f;

	// --------------------------------------------------------------------------------
	// Materials - which materials paint the terrain, and where each one kicks in.
	// --------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 0 Material"))
	TSoftObjectPtr<UMaterialInterface> Layer0Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/AerialGrassRock/MI_AerialGrassRock.MI_AerialGrassRock"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 1 Material"))
	TSoftObjectPtr<UMaterialInterface> Layer1Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/BrownMudRocks/MI_BrownMudRocks.MI_BrownMudRocks"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 2 Material"))
	TSoftObjectPtr<UMaterialInterface> Layer2Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/CoralMud/MI_CoralMud.MI_CoralMud"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 3 Material"))
	TSoftObjectPtr<UMaterialInterface> Layer3Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/Snow/MI_Snow.MI_Snow"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials|Height Splitter", meta=(DisplayName="Layer 1 Height", ToolTip="Move this up or down to match wherever you want Layer 1 to show up."))
	float Layer1Height = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials|Height Splitter", meta=(DisplayName="Layer 1 Blend", ClampMin="0.1", ToolTip="Raise this for a longer, softer blend into Layer 1. Lower it for a sharper cutoff."))
	float Layer1Blend = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials|Height Splitter", meta=(DisplayName="Layer 2 Height", ToolTip="Move this up or down to match wherever you want Layer 2 to show up."))
	float Layer2Height = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials|Height Splitter", meta=(DisplayName="Layer 2 Blend", ClampMin="0.1", ToolTip="Raise this for a longer, softer blend into Layer 2. Lower it for a sharper cutoff."))
	float Layer2Blend = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials|Height Splitter", meta=(DisplayName="Layer 3 Height", ToolTip="Move this up or down to match wherever you want Layer 3 to show up."))
	float Layer3Height = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials|Height Splitter", meta=(DisplayName="Layer 3 Blend", ClampMin="0.1", ToolTip="Raise this for a longer, softer blend into Layer 3. Lower it for a sharper cutoff."))
	float Layer3Blend = 5.0f;

	UVG_Alfheim();
	virtual TVoxelSharedRef<FVoxelTransformableGeneratorInstance> GetTransformableInstance() override;
};