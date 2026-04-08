// Lost Crow Dev - https://github.com/lostcrowdev/VoxelPluginFreeLegacy-Alfheim
#pragma once

#include "CoreMinimal.h"
#include "VoxelGeneratedWorldGeneratorsIncludes.h"
#include "VG_Alfheim.generated.h"

UCLASS(Blueprintable)
class UVG_Alfheim : public UVoxelGraphGeneratorHelper
{
    GENERATED_BODY()
    
public:
    // Terrain Generation Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Generation", meta=(DisplayName="Master Seed"))
    int32 Master_Seed = 1337;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain Generation", meta=(DisplayName="Terrain Height Scale", ClampMin="1.0", ClampMax="10000.0"))
    float Terrain_Height_Scale = 300.0f;
    
    // Base Terrain Noise Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base Terrain Noise", meta=(DisplayName="Base Frequency", ClampMin="0.0001", ClampMax="1.0"))
    float Base_Frequency = 0.002f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base Terrain Noise", meta=(DisplayName="Base Octaves", ClampMin="1", ClampMax="10"))
    int32 Base_Octaves = 7;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base Terrain Noise", meta=(DisplayName="Base Gain", ClampMin="0.0", ClampMax="1.0"))
    float Base_Gain = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base Terrain Noise", meta=(DisplayName="Base Lacunarity", ClampMin="1.0", ClampMax="4.0"))
    float Base_Lacunarity = 2.0f;
    
    // Material Noise Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Noise", meta=(DisplayName="Material Frequency", ClampMin="0.0001", ClampMax="1.0"))
    float Material_Frequency = 0.02f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Noise", meta=(DisplayName="Material Octaves", ClampMin="1", ClampMax="10"))
    int32 Material_Octaves = 7;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Noise", meta=(DisplayName="Material Gain", ClampMin="0.0", ClampMax="1.0"))
    float Material_Gain = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Noise", meta=(DisplayName="Material Lacunarity", ClampMin="1.0", ClampMax="4.0"))
    float Material_Lacunarity = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Noise", meta=(DisplayName="Material Variation Strength", ClampMin="0.0", ClampMax="100.0"))
    float Material_Variation_Strength = 25.0f;
    
    // Height Splitter Parameters (Material Layer Heights) (glitch / not working correctly)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Layers", meta=(DisplayName="Layer 1 Height"))
    float Layer_1_Height = -75.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Layers", meta=(DisplayName="Layer 1 Blend", ClampMin="0.1", ClampMax="50.0"))
    float Layer_1_Blend = 5.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Layers", meta=(DisplayName="Layer 2 Height"))
    float Layer_2_Height = -20.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Layers", meta=(DisplayName="Layer 2 Blend", ClampMin="0.1", ClampMax="50.0"))
    float Layer_2_Blend = 5.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Layers", meta=(DisplayName="Layer 3 Height"))
    float Layer_3_Height = 10.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Layers", meta=(DisplayName="Layer 3 Blend", ClampMin="0.1", ClampMax="50.0"))
    float Layer_3_Blend = 5.0f;
    
    // Island Mode Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Mode", meta=(DisplayName="Enable Island Mode"))
    bool Enable_Island_Mode = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Mode", meta=(DisplayName="Island Radius", ClampMin="100.0", ClampMax="10000.0", EditCondition="Enable_Island_Mode", ToolTip="Distance from center (0,0) to where the island edge begins"))
    float Island_Radius = 2500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Mode", meta=(DisplayName="Island Falloff Distance", ClampMin="50.0", ClampMax="5000.0", EditCondition="Enable_Island_Mode", ToolTip="How far the shoreline/beach transition extends from the island edge"))
    float Island_Falloff_Distance = 800.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Mode", meta=(DisplayName="Island Falloff Strength", ClampMin="0.1", ClampMax="5.0", EditCondition="Enable_Island_Mode", ToolTip="Controls transition sharpness (1.0 = linear, higher = sharper cliff)"))
    float Island_Falloff_Strength = 1.5f;
    
    // Beach Plateau Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach Plateau", meta=(DisplayName="Enable Beach Plateau", ToolTip="Flattens terrain globally wherever its surface height falls within Beach_Plateau_Height_Range of Beach_Plateau_Target_Height. Works across the entire map, creating a beach shelf on every island shoreline."))
    bool Enable_Beach_Plateau = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach Plateau", meta=(DisplayName="Beach Plateau Target Height", ToolTip="The world height the beach is flattened toward. Set this to your waterline height."))
    float Beach_Plateau_Target_Height = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach Plateau", meta=(DisplayName="Beach Plateau Height Range", ClampMin="1.0", ClampMax="500.0", ToolTip="How many units above and below the target height the flattening effect reaches. Terrain at exactly the target height is fully flattened; terrain beyond this range is unaffected."))
    float Beach_Plateau_Height_Range = 40.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Beach Plateau", meta=(DisplayName="Beach Plateau Blend Strength", ClampMin="0.1", ClampMax="10.0", ToolTip="Controls how sharply terrain snaps to the plateau. 1.0 = gradual smooth blend; higher values produce a flatter, more shelf-like result."))
    float Beach_Plateau_Blend_Strength = 2.0f;

    // Mountain Plateau Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountain Plateau", meta=(DisplayName="Enable Mountain Plateau", ToolTip="Caps terrain peaks above Mountain_Plateau_Target_Height, pulling them down to form a flat elevated plateau. Only affects terrain above the target height - never disturbs lower terrain or beaches."))
    bool Enable_Mountain_Plateau = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountain Plateau", meta=(DisplayName="Mountain Plateau Target Height", ToolTip="Peaks above this height get flattened down toward it. Should be set above Beach Plateau Target Height and above typical plains level."))
    float Mountain_Plateau_Target_Height = 80.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountain Plateau", meta=(DisplayName="Mountain Plateau Height Range", ClampMin="1.0", ClampMax="500.0", ToolTip="How far above the target height the flattening effect reaches. A peak that is Range units above the target is unaffected; peaks closer than Range get pulled down."))
    float Mountain_Plateau_Height_Range = 40.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountain Plateau", meta=(DisplayName="Mountain Plateau Blend Strength", ClampMin="0.1", ClampMax="10.0", ToolTip="Controls how sharply peaks snap to the plateau. 1.0 = gradual; higher values produce a flatter, more table-top result."))
    float Mountain_Plateau_Blend_Strength = 2.0f;
    
    // Material References -- Glitchy / not working
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 0"))
    TSoftObjectPtr<UMaterialInterface> Layer_0 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/AerialGrassRock/MI_AerialGrassRock.MI_AerialGrassRock"));
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 1"))
    TSoftObjectPtr<UMaterialInterface> Layer_1 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/BrownMudRocks/MI_BrownMudRocks.MI_BrownMudRocks"));
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 2"))
    TSoftObjectPtr<UMaterialInterface> Layer_2 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/CoralMud/MI_CoralMud.MI_CoralMud"));
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials", meta=(DisplayName="Layer 3"))
    TSoftObjectPtr<UMaterialInterface> Layer_3 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/Snow/MI_Snow.MI_Snow"));
    
    UVG_Alfheim();
    virtual TVoxelSharedRef<FVoxelTransformableGeneratorInstance> GetTransformableInstance() override;
};