// Lost Crow Dev - https://github.com/lostcrowdev/VoxelPluginFreeLegacy-Alfheim

#include "VG_Alfheim.h"

PRAGMA_GENERATED_VOXEL_GRAPH_START

using FVoxelGraphSeed = int32;

#if VOXEL_GRAPH_GENERATED_VERSION == 1
class FVG_Alfheim : public TVoxelGraphGeneratorInstanceHelper<FVG_Alfheim, UVG_Alfheim>
{
public:
	struct FParams
	{
		const FName Layer_0;
		const FName Layer_1;
		const FName Layer_2;
		const FName Layer_3;
		const int32 Master_Seed;
		const float Terrain_Height_Scale;
		const float Base_Frequency;
		const int32 Base_Octaves;
		const float Base_Gain;
		const float Base_Lacunarity;
		const float Material_Frequency;
		const int32 Material_Octaves;
		const float Material_Gain;
		const float Material_Lacunarity;
		const float Material_Variation_Strength;
		const float Layer_1_Height;
		const float Layer_1_Blend;
		const float Layer_2_Height;
		const float Layer_2_Blend;
		const float Layer_3_Height;
		const float Layer_3_Blend;
		const bool Enable_Island_Mode;
		const float Island_Radius;
		const float Island_Falloff_Distance;
		const float Island_Falloff_Strength;
	};
	
	class FLocalComputeStruct_LocalValue
	{
	public:
		struct FOutputs
		{
			FOutputs() {}
			
			void Init(const FVoxelGraphOutputsInit& Init)
			{
			}
			
			template<typename T, uint32 Index>
			T Get() const;
			template<typename T, uint32 Index>
			void Set(T Value);
			
			v_flt Value;
		};
		struct FBufferConstant
		{
			FBufferConstant() {}
			
		};
		
		struct FBufferX
		{
			FBufferX() {}
			
			v_flt Variable_2; // X output 0
		};
		
		struct FBufferXY
		{
			FBufferXY() {}
			
			v_flt Variable_0; // 2D Noise SDF.+ output 0
		};
		
		FLocalComputeStruct_LocalValue(const FParams& InParams)
			: Params(InParams)
		{
		}
		
		void Init(const FVoxelGeneratorInit& InitStruct)
		{
			////////////////////////////////////////////////////
			//////////////////// Init nodes ////////////////////
			////////////////////////////////////////////////////
			{
				////////////////////////////////////////////////////
				/////////////// Constant nodes init ////////////////
				////////////////////////////////////////////////////
				{
					/////////////////////////////////////////////////////////////////////////////////
					//////// First compute all seeds in case they are used by constant nodes ////////
					/////////////////////////////////////////////////////////////////////////////////
					
					// Init of Make Seeds - NOW USING MASTER SEED
					FVoxelGraphSeed Variable_5; // Make Seeds output 0
					FVoxelGraphSeed Make_Seeds_0_Temp_1; // Make Seeds output 1
					Variable_5 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Master_Seed));
					Make_Seeds_0_Temp_1 = FVoxelUtilities::MurmurHash32(Variable_5);
					
					
					////////////////////////////////////////////////////
					///////////// Then init constant nodes /////////////
					////////////////////////////////////////////////////
					
				}
				
				////////////////////////////////////////////////////
				//////////////////// Other inits ///////////////////
				////////////////////////////////////////////////////
				Function0_XYZWithoutCache_Init(InitStruct);
			}
			
			////////////////////////////////////////////////////
			//////////////// Compute constants /////////////////
			////////////////////////////////////////////////////
			{
			}
		}
		void ComputeX(const FVoxelContext& Context, FBufferX& BufferX) const
		{
			Function0_X_Compute(Context, BufferX);
		}
		void ComputeXYWithCache(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			Function0_XYWithCache_Compute(Context, BufferX, BufferXY);
		}
		void ComputeXYWithoutCache(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			Function0_XYWithoutCache_Compute(Context, BufferX, BufferXY);
		}
		void ComputeXYZWithCache(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
			Function0_XYZWithCache_Compute(Context, BufferX, BufferXY, Outputs);
		}
		void ComputeXYZWithoutCache(const FVoxelContext& Context, FOutputs& Outputs) const
		{
			Function0_XYZWithoutCache_Compute(Context, Outputs);
		}
		
		inline FBufferX GetBufferX() const { return {}; }
		inline FBufferXY GetBufferXY() const { return {}; }
		inline FOutputs GetOutputs() const { return {}; }
		
	private:
		FBufferConstant BufferConstant;
		
		const FParams& Params;
		
		FVoxelFastNoise _2D_Perlin_Noise_Fractal_0_Noise;
		TStaticArray<uint8, 32> _2D_Perlin_Noise_Fractal_0_LODToOctaves;
		
		///////////////////////////////////////////////////////////////////////
		//////////////////////////// Init functions ///////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
			// Init of Make Seeds - NOW USING MASTER SEED
			FVoxelGraphSeed Variable_5; // Make Seeds output 0
			FVoxelGraphSeed Make_Seeds_0_Temp_1; // Make Seeds output 1
			Variable_5 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Master_Seed));
			Make_Seeds_0_Temp_1 = FVoxelUtilities::MurmurHash32(Variable_5);
			
			// Init of 2D Perlin Noise Fractal - NOW USING PARAMETERS
			_2D_Perlin_Noise_Fractal_0_Noise.SetSeed(Variable_5);
			_2D_Perlin_Noise_Fractal_0_Noise.SetInterpolation(EVoxelNoiseInterpolation::Quintic);
			_2D_Perlin_Noise_Fractal_0_Noise.SetFractalOctavesAndGain(Params.Base_Octaves, Params.Base_Gain);
			_2D_Perlin_Noise_Fractal_0_Noise.SetFractalLacunarity(Params.Base_Lacunarity);
			_2D_Perlin_Noise_Fractal_0_Noise.SetFractalType(EVoxelNoiseFractalType::FBM);
			
			// Set octaves for all LOD levels
			for (int32 i = 0; i < 32; i++)
			{
				_2D_Perlin_Noise_Fractal_0_LODToOctaves[i] = Params.Base_Octaves;
			}
		}
		
		///////////////////////////////////////////////////////////////////////
		////////////////////////// Compute functions //////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_X_Compute(const FVoxelContext& Context, FBufferX& BufferX) const
		{
			// X
			BufferX.Variable_2 = Context.GetLocalX();
		}
		
		void Function0_XYWithCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			// Y
			v_flt Variable_3; // Y output 0
			Variable_3 = Context.GetLocalY();
			
			// 2D Perlin Noise Fractal - NOW USING FREQUENCY PARAMETER
			v_flt Variable_1; // 2D Perlin Noise Fractal output 0
			Variable_1 = _2D_Perlin_Noise_Fractal_0_Noise.GetPerlinFractal_2D(BufferX.Variable_2, Variable_3, v_flt(Params.Base_Frequency), _2D_Perlin_Noise_Fractal_0_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_1 = FMath::Clamp<v_flt>(Variable_1, -0.601283, 0.600269);
			
			// 2D Noise SDF.* - NOW USING HEIGHT SCALE PARAMETER
			v_flt Variable_8; // 2D Noise SDF.* output 0
			Variable_8 = Variable_1 * v_flt(Params.Terrain_Height_Scale);
			
			// 2D Noise SDF.+
			BufferXY.Variable_0 = Variable_8 + v_flt(0.0f);
		}
		
		void Function0_XYWithoutCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			// Y
			v_flt Variable_3; // Y output 0
			Variable_3 = Context.GetLocalY();
			
			// X
			BufferX.Variable_2 = Context.GetLocalX();
			
			// 2D Perlin Noise Fractal - NOW USING FREQUENCY PARAMETER
			v_flt Variable_1; // 2D Perlin Noise Fractal output 0
			Variable_1 = _2D_Perlin_Noise_Fractal_0_Noise.GetPerlinFractal_2D(BufferX.Variable_2, Variable_3, v_flt(Params.Base_Frequency), _2D_Perlin_Noise_Fractal_0_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_1 = FMath::Clamp<v_flt>(Variable_1, -0.601283, 0.600269);
			
			// 2D Noise SDF.* - NOW USING HEIGHT SCALE PARAMETER
			v_flt Variable_8; // 2D Noise SDF.* output 0
			Variable_8 = Variable_1 * v_flt(Params.Terrain_Height_Scale);
			
			// 2D Noise SDF.+
			BufferXY.Variable_0 = Variable_8 + v_flt(0.0f);
		}
		
		void Function0_XYZWithCache_Compute(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
			// Z
			v_flt Variable_4; // Z output 0
			Variable_4 = Context.GetLocalZ();
			
			// 2D Noise SDF.-
			v_flt Variable_7; // 2D Noise SDF.- output 0
			Variable_7 = Variable_4 - BufferXY.Variable_0;
			
			// ISLAND MODE: Apply circular falloff if enabled
			if (Params.Enable_Island_Mode)
			{
				// Calculate distance from center in XY plane
				v_flt DistanceFromCenter = FMath::Sqrt(BufferX.Variable_2 * BufferX.Variable_2 + Context.GetLocalY() * Context.GetLocalY());
				
				// Calculate falloff
				v_flt FalloffStart = Params.Island_Radius;
				v_flt FalloffEnd = Params.Island_Radius + Params.Island_Falloff_Distance;
				
				// Smooth step falloff
				v_flt FalloffAmount = FVoxelMathNodeFunctions::SmoothStep(FalloffStart, FalloffEnd, DistanceFromCenter);
				
				// Apply falloff strength
				FalloffAmount = FMath::Pow(FalloffAmount, Params.Island_Falloff_Strength);
				
				// Push terrain down at edges (positive values in SDF = empty space)
				v_flt HeightReduction = FalloffAmount * (Params.Terrain_Height_Scale * 2.0f);
				Variable_7 = Variable_7 + HeightReduction;
			}
			
			// Set High Quality Value.*
			v_flt Variable_6; // Set High Quality Value.* output 0
			Variable_6 = Variable_7 * v_flt(0.2f);
			
			Outputs.Value = Variable_6;
		}
		
		void Function0_XYZWithoutCache_Compute(const FVoxelContext& Context, FOutputs& Outputs) const
		{
			// Z
			v_flt Variable_4; // Z output 0
			Variable_4 = Context.GetLocalZ();
			
			// Y
			v_flt Variable_3; // Y output 0
			Variable_3 = Context.GetLocalY();
			
			// X
			v_flt Variable_2; // X output 0
			Variable_2 = Context.GetLocalX();
			
			// 2D Perlin Noise Fractal - NOW USING FREQUENCY PARAMETER
			v_flt Variable_1; // 2D Perlin Noise Fractal output 0
			Variable_1 = _2D_Perlin_Noise_Fractal_0_Noise.GetPerlinFractal_2D(Variable_2, Variable_3, v_flt(Params.Base_Frequency), _2D_Perlin_Noise_Fractal_0_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_1 = FMath::Clamp<v_flt>(Variable_1, -0.601283, 0.600269);
			
			// 2D Noise SDF.* - NOW USING HEIGHT SCALE PARAMETER
			v_flt Variable_8; // 2D Noise SDF.* output 0
			Variable_8 = Variable_1 * v_flt(Params.Terrain_Height_Scale);
			
			// 2D Noise SDF.+
			v_flt Variable_0; // 2D Noise SDF.+ output 0
			Variable_0 = Variable_8 + v_flt(0.0f);
			
			// 2D Noise SDF.-
			v_flt Variable_7; // 2D Noise SDF.- output 0
			Variable_7 = Variable_4 - Variable_0;
			
			// ISLAND MODE: Apply circular falloff if enabled
			if (Params.Enable_Island_Mode)
			{
				// Calculate distance from center in XY plane
				v_flt DistanceFromCenter = FMath::Sqrt(Variable_2 * Variable_2 + Variable_3 * Variable_3);
				
				// Calculate falloff
				v_flt FalloffStart = Params.Island_Radius;
				v_flt FalloffEnd = Params.Island_Radius + Params.Island_Falloff_Distance;
				
				// Smooth step falloff
				v_flt FalloffAmount = FVoxelMathNodeFunctions::SmoothStep(FalloffStart, FalloffEnd, DistanceFromCenter);
				
				// Apply falloff strength
				FalloffAmount = FMath::Pow(FalloffAmount, Params.Island_Falloff_Strength);
				
				// Push terrain down at edges (positive values in SDF = empty space)
				v_flt HeightReduction = FalloffAmount * (Params.Terrain_Height_Scale * 2.0f);
				Variable_7 = Variable_7 + HeightReduction;
			}
			
			// Set High Quality Value.*
			v_flt Variable_6; // Set High Quality Value.* output 0
			Variable_6 = Variable_7 * v_flt(0.2f);
			
			Outputs.Value = Variable_6;
		}
	};
	
	class FLocalComputeStruct_LocalMaterial
	{
	public:
		struct FOutputs
		{
			FOutputs() {}
			
			void Init(const FVoxelGraphOutputsInit& Init)
			{
				MaterialBuilder.SetMaterialConfig(Init.MaterialConfig);
			}
			
			template<typename T, uint32 Index>
			T Get() const;
			template<typename T, uint32 Index>
			void Set(T Value);
			
			FVoxelMaterialBuilder MaterialBuilder;
		};
		struct FBufferConstant
		{
			FBufferConstant() {}
			
			int32 Variable_6; // Get Material Collection Index: Layer 1 output 0
			int32 Variable_7; // Get Material Collection Index: Layer 2 output 0
			int32 Variable_8; // Get Material Collection Index: Layer 3 output 0
			int32 Variable_0; // Get Material Collection Index: Layer 0 output 0
		};
		
		struct FBufferX
		{
			FBufferX() {}
			
			v_flt Variable_11; // X output 0
		};
		
		struct FBufferXY
		{
			FBufferXY() {}
			
			v_flt Variable_14; // * output 0
		};
		
		FLocalComputeStruct_LocalMaterial(const FParams& InParams)
			: Params(InParams)
		{
		}
		
		void Init(const FVoxelGeneratorInit& InitStruct)
		{
			////////////////////////////////////////////////////
			//////////////////// Init nodes ////////////////////
			////////////////////////////////////////////////////
			{
				////////////////////////////////////////////////////
				/////////////// Constant nodes init ////////////////
				////////////////////////////////////////////////////
				{
					/////////////////////////////////////////////////////////////////////////////////
					//////// First compute all seeds in case they are used by constant nodes ////////
					/////////////////////////////////////////////////////////////////////////////////
					
					// Init of Make Seeds - NOW USING MASTER SEED
					FVoxelGraphSeed Make_Seeds_1_Temp_0; // Make Seeds output 0
					FVoxelGraphSeed Variable_10; // Make Seeds output 1
					Make_Seeds_1_Temp_0 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Master_Seed));
					Variable_10 = FVoxelUtilities::MurmurHash32(Make_Seeds_1_Temp_0);
					
					
					////////////////////////////////////////////////////
					///////////// Then init constant nodes /////////////
					////////////////////////////////////////////////////
					
					// Init of Get Material Collection Index: Layer 1
					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_1_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer_1);
					}
					else
					{
						Get_Material_Collection_Index__Layer_1_0_Index = -1;
					}
					
					// Init of Get Material Collection Index: Layer 2
					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_2_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer_2);
					}
					else
					{
						Get_Material_Collection_Index__Layer_2_0_Index = -1;
					}
					
					// Init of Get Material Collection Index: Layer 3
					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_3_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer_3);
					}
					else
					{
						Get_Material_Collection_Index__Layer_3_0_Index = -1;
					}
					
					// Init of Get Material Collection Index: Layer 0
					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_0_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer_0);
					}
					else
					{
						Get_Material_Collection_Index__Layer_0_0_Index = -1;
					}
				}
				
				////////////////////////////////////////////////////
				//////////////////// Other inits ///////////////////
				////////////////////////////////////////////////////
				Function0_XYZWithoutCache_Init(InitStruct);
			}
			
			////////////////////////////////////////////////////
			//////////////// Compute constants /////////////////
			////////////////////////////////////////////////////
			{
				// Get Material Collection Index: Layer 1
				BufferConstant.Variable_6 = Get_Material_Collection_Index__Layer_1_0_Index;
				
				// Get Material Collection Index: Layer 2
				BufferConstant.Variable_7 = Get_Material_Collection_Index__Layer_2_0_Index;
				
				// Get Material Collection Index: Layer 3
				BufferConstant.Variable_8 = Get_Material_Collection_Index__Layer_3_0_Index;
				
				// Get Material Collection Index: Layer 0
				BufferConstant.Variable_0 = Get_Material_Collection_Index__Layer_0_0_Index;
			}
		}
		void ComputeX(const FVoxelContext& Context, FBufferX& BufferX) const
		{
			Function0_X_Compute(Context, BufferX);
		}
		void ComputeXYWithCache(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			Function0_XYWithCache_Compute(Context, BufferX, BufferXY);
		}
		void ComputeXYWithoutCache(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			Function0_XYWithoutCache_Compute(Context, BufferX, BufferXY);
		}
		void ComputeXYZWithCache(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
			Function0_XYZWithCache_Compute(Context, BufferX, BufferXY, Outputs);
		}
		void ComputeXYZWithoutCache(const FVoxelContext& Context, FOutputs& Outputs) const
		{
			Function0_XYZWithoutCache_Compute(Context, Outputs);
		}
		
		inline FBufferX GetBufferX() const { return {}; }
		inline FBufferXY GetBufferXY() const { return {}; }
		inline FOutputs GetOutputs() const { return {}; }
		
	private:
		FBufferConstant BufferConstant;
		
		const FParams& Params;
		
		int32 Get_Material_Collection_Index__Layer_1_0_Index;
		int32 Get_Material_Collection_Index__Layer_2_0_Index;
		int32 Get_Material_Collection_Index__Layer_3_0_Index;
		int32 Get_Material_Collection_Index__Layer_0_0_Index;
		FVoxelFastNoise _2D_Perlin_Noise_Fractal_1_Noise;
		TStaticArray<uint8, 32> _2D_Perlin_Noise_Fractal_1_LODToOctaves;
		
		///////////////////////////////////////////////////////////////////////
		//////////////////////////// Init functions ///////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
			// Init of Make Seeds - NOW USING MASTER SEED
			FVoxelGraphSeed Make_Seeds_1_Temp_0; // Make Seeds output 0
			FVoxelGraphSeed Variable_10; // Make Seeds output 1
			Make_Seeds_1_Temp_0 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Master_Seed));
			Variable_10 = FVoxelUtilities::MurmurHash32(Make_Seeds_1_Temp_0);
			
			// Init of 2D Perlin Noise Fractal - NOW USING MATERIAL PARAMETERS
			_2D_Perlin_Noise_Fractal_1_Noise.SetSeed(Variable_10);
			_2D_Perlin_Noise_Fractal_1_Noise.SetInterpolation(EVoxelNoiseInterpolation::Quintic);
			_2D_Perlin_Noise_Fractal_1_Noise.SetFractalOctavesAndGain(Params.Material_Octaves, Params.Material_Gain);
			_2D_Perlin_Noise_Fractal_1_Noise.SetFractalLacunarity(Params.Material_Lacunarity);
			_2D_Perlin_Noise_Fractal_1_Noise.SetFractalType(EVoxelNoiseFractalType::FBM);
			
			// Set octaves for all LOD levels
			for (int32 i = 0; i < 32; i++)
			{
				_2D_Perlin_Noise_Fractal_1_LODToOctaves[i] = Params.Material_Octaves;
			}
		}
		
		///////////////////////////////////////////////////////////////////////
		////////////////////////// Compute functions //////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_X_Compute(const FVoxelContext& Context, FBufferX& BufferX) const
		{
			// X
			BufferX.Variable_11 = Context.GetLocalX();
		}
		
		void Function0_XYWithCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			// Y
			v_flt Variable_12; // Y output 0
			Variable_12 = Context.GetLocalY();
			
			// 2D Perlin Noise Fractal - NOW USING MATERIAL FREQUENCY PARAMETER
			v_flt Variable_13; // 2D Perlin Noise Fractal output 0
			Variable_13 = _2D_Perlin_Noise_Fractal_1_Noise.GetPerlinFractal_2D(BufferX.Variable_11, Variable_12, v_flt(Params.Material_Frequency), _2D_Perlin_Noise_Fractal_1_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_13 = FMath::Clamp<v_flt>(Variable_13, -0.306139, 0.328394);
			
			// * - NOW USING MATERIAL VARIATION STRENGTH PARAMETER
			BufferXY.Variable_14 = Variable_13 * v_flt(Params.Material_Variation_Strength);
		}
		
		void Function0_XYWithoutCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			// Y
			v_flt Variable_12; // Y output 0
			Variable_12 = Context.GetLocalY();
			
			// X
			BufferX.Variable_11 = Context.GetLocalX();
			
			// 2D Perlin Noise Fractal - NOW USING MATERIAL FREQUENCY PARAMETER
			v_flt Variable_13; // 2D Perlin Noise Fractal output 0
			Variable_13 = _2D_Perlin_Noise_Fractal_1_Noise.GetPerlinFractal_2D(BufferX.Variable_11, Variable_12, v_flt(Params.Material_Frequency), _2D_Perlin_Noise_Fractal_1_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_13 = FMath::Clamp<v_flt>(Variable_13, -0.306139, 0.328394);
			
			// * - NOW USING MATERIAL VARIATION STRENGTH PARAMETER
			BufferXY.Variable_14 = Variable_13 * v_flt(Params.Material_Variation_Strength);
		}
		
		void Function0_XYZWithCache_Compute(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
			// Z
			v_flt Variable_5; // Z output 0
			Variable_5 = Context.GetLocalZ();
			
			// +
			v_flt Variable_9; // + output 0
			Variable_9 = Variable_5 + BufferXY.Variable_14;
			
			// Height Splitter - NOW USING LAYER HEIGHT PARAMETERS
			v_flt Variable_1; // Height Splitter output 0
			v_flt Variable_2; // Height Splitter output 1
			v_flt Variable_3; // Height Splitter output 2
			v_flt Variable_4; // Height Splitter output 3
			{
				TVoxelStaticArray<v_flt, 6> InputsArray;
				TVoxelStaticArray<v_flt, 4> OutputsArray;
				InputsArray[0] = v_flt(Params.Layer_1_Height);
				InputsArray[1] = v_flt(Params.Layer_1_Blend);
				InputsArray[2] = v_flt(Params.Layer_2_Height);
				InputsArray[3] = v_flt(Params.Layer_2_Blend);
				InputsArray[4] = v_flt(Params.Layer_3_Height);
				InputsArray[5] = v_flt(Params.Layer_3_Blend);
				FVoxelMathNodeFunctions::HeightSplit(Variable_9, InputsArray, OutputsArray);
				Variable_1 = OutputsArray[0];
				Variable_2 = OutputsArray[1];
				Variable_3 = OutputsArray[2];
				Variable_4 = OutputsArray[3];
			}
			
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_0, Variable_1, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_6, Variable_2, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_7, Variable_3, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_8, Variable_4, bool(false));
		}
		
		void Function0_XYZWithoutCache_Compute(const FVoxelContext& Context, FOutputs& Outputs) const
		{
			// Y
			v_flt Variable_12; // Y output 0
			Variable_12 = Context.GetLocalY();
			
			// Z
			v_flt Variable_5; // Z output 0
			Variable_5 = Context.GetLocalZ();
			
			// X
			v_flt Variable_11; // X output 0
			Variable_11 = Context.GetLocalX();
			
			// 2D Perlin Noise Fractal - NOW USING MATERIAL FREQUENCY PARAMETER
			v_flt Variable_13; // 2D Perlin Noise Fractal output 0
			Variable_13 = _2D_Perlin_Noise_Fractal_1_Noise.GetPerlinFractal_2D(Variable_11, Variable_12, v_flt(Params.Material_Frequency), _2D_Perlin_Noise_Fractal_1_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_13 = FMath::Clamp<v_flt>(Variable_13, -0.306139, 0.328394);
			
			// * - NOW USING MATERIAL VARIATION STRENGTH PARAMETER
			v_flt Variable_14; // * output 0
			Variable_14 = Variable_13 * v_flt(Params.Material_Variation_Strength);
			
			// +
			v_flt Variable_9; // + output 0
			Variable_9 = Variable_5 + Variable_14;
			
			// Height Splitter - NOW USING LAYER HEIGHT PARAMETERS
			v_flt Variable_1; // Height Splitter output 0
			v_flt Variable_2; // Height Splitter output 1
			v_flt Variable_3; // Height Splitter output 2
			v_flt Variable_4; // Height Splitter output 3
			{
				TVoxelStaticArray<v_flt, 6> InputsArray;
				TVoxelStaticArray<v_flt, 4> OutputsArray;
				InputsArray[0] = v_flt(Params.Layer_1_Height);
				InputsArray[1] = v_flt(Params.Layer_1_Blend);
				InputsArray[2] = v_flt(Params.Layer_2_Height);
				InputsArray[3] = v_flt(Params.Layer_2_Blend);
				InputsArray[4] = v_flt(Params.Layer_3_Height);
				InputsArray[5] = v_flt(Params.Layer_3_Blend);
				FVoxelMathNodeFunctions::HeightSplit(Variable_9, InputsArray, OutputsArray);
				Variable_1 = OutputsArray[0];
				Variable_2 = OutputsArray[1];
				Variable_3 = OutputsArray[2];
				Variable_4 = OutputsArray[3];
			}
			
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_0, Variable_1, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_6, Variable_2, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_7, Variable_3, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_8, Variable_4, bool(false));
		}
	};
	
	class FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ
	{
	public:
		struct FOutputs
		{
			FOutputs() {}
			
			void Init(const FVoxelGraphOutputsInit& Init)
			{
			}
			
			template<typename T, uint32 Index>
			T Get() const;
			template<typename T, uint32 Index>
			void Set(T Value);
			
			v_flt UpVectorX;
			v_flt UpVectorY;
			v_flt UpVectorZ;
		};
		struct FBufferConstant
		{
			FBufferConstant() {}
		};
		
		struct FBufferX
		{
			FBufferX() {}
		};
		
		struct FBufferXY
		{
			FBufferXY() {}
		};
		
		FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ(const FParams& InParams)
			: Params(InParams)
		{
		}
		
		void Init(const FVoxelGeneratorInit& InitStruct)
		{
			////////////////////////////////////////////////////
			//////////////////// Init nodes ////////////////////
			////////////////////////////////////////////////////
			{
				////////////////////////////////////////////////////
				/////////////// Constant nodes init ////////////////
				////////////////////////////////////////////////////
				{
					/////////////////////////////////////////////////////////////////////////////////
					//////// First compute all seeds in case they are used by constant nodes ////////
					/////////////////////////////////////////////////////////////////////////////////
					
					
					////////////////////////////////////////////////////
					///////////// Then init constant nodes /////////////
					////////////////////////////////////////////////////
				}
				
				////////////////////////////////////////////////////
				//////////////////// Other inits ///////////////////
				////////////////////////////////////////////////////
				Function0_XYZWithoutCache_Init(InitStruct);
			}
			
			////////////////////////////////////////////////////
			//////////////// Compute constants /////////////////
			////////////////////////////////////////////////////
			{
			}
		}
		void ComputeX(const FVoxelContext& Context, FBufferX& BufferX) const
		{
			Function0_X_Compute(Context, BufferX);
		}
		void ComputeXYWithCache(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			Function0_XYWithCache_Compute(Context, BufferX, BufferXY);
		}
		void ComputeXYWithoutCache(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			Function0_XYWithoutCache_Compute(Context, BufferX, BufferXY);
		}
		void ComputeXYZWithCache(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
			Function0_XYZWithCache_Compute(Context, BufferX, BufferXY, Outputs);
		}
		void ComputeXYZWithoutCache(const FVoxelContext& Context, FOutputs& Outputs) const
		{
			Function0_XYZWithoutCache_Compute(Context, Outputs);
		}
		
		inline FBufferX GetBufferX() const { return {}; }
		inline FBufferXY GetBufferXY() const { return {}; }
		inline FOutputs GetOutputs() const { return {}; }
		
	private:
		FBufferConstant BufferConstant;
		
		const FParams& Params;
		
		///////////////////////////////////////////////////////////////////////
		//////////////////////////// Init functions ///////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
		}
		
		///////////////////////////////////////////////////////////////////////
		////////////////////////// Compute functions //////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_X_Compute(const FVoxelContext& Context, FBufferX& BufferX) const
		{
		}
		
		void Function0_XYWithCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
		}
		
		void Function0_XYWithoutCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
		}
		
		void Function0_XYZWithCache_Compute(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
		}
		
		void Function0_XYZWithoutCache_Compute(const FVoxelContext& Context, FOutputs& Outputs) const
		{
		}
	};
	
	class FLocalComputeStruct_LocalValueRangeAnalysis
	{
	public:
		struct FOutputs
		{
			FOutputs() {}
			
			void Init(const FVoxelGraphOutputsInit& Init)
			{
			}
			
			template<typename T, uint32 Index>
			TVoxelRange<T> Get() const;
			template<typename T, uint32 Index>
			void Set(TVoxelRange<T> Value);
			
			TVoxelRange<v_flt> Value;
		};
		struct FBufferConstant
		{
			FBufferConstant() {}
			
			TVoxelRange<v_flt> Variable_0; // 2D Noise SDF.+ output 0
		};
		
		struct FBufferX
		{
			FBufferX() {}
		};
		
		struct FBufferXY
		{
			FBufferXY() {}
		};
		
		FLocalComputeStruct_LocalValueRangeAnalysis(const FParams& InParams)
			: Params(InParams)
		{
		}
		
		void Init(const FVoxelGeneratorInit& InitStruct)
		{
			////////////////////////////////////////////////////
			//////////////////// Init nodes ////////////////////
			////////////////////////////////////////////////////
			{
				////////////////////////////////////////////////////
				/////////////// Constant nodes init ////////////////
				////////////////////////////////////////////////////
				{
					/////////////////////////////////////////////////////////////////////////////////
					//////// First compute all seeds in case they are used by constant nodes ////////
					/////////////////////////////////////////////////////////////////////////////////
					
					
					////////////////////////////////////////////////////
					///////////// Then init constant nodes /////////////
					////////////////////////////////////////////////////
					
					// Init of 2D Perlin Noise Fractal - NOW USING PARAMETERS
					_2D_Perlin_Noise_Fractal_2_Noise.SetSeed(FVoxelGraphSeed(Params.Master_Seed));
					_2D_Perlin_Noise_Fractal_2_Noise.SetInterpolation(EVoxelNoiseInterpolation::Quintic);
					_2D_Perlin_Noise_Fractal_2_Noise.SetFractalOctavesAndGain(Params.Base_Octaves, Params.Base_Gain);
					_2D_Perlin_Noise_Fractal_2_Noise.SetFractalLacunarity(Params.Base_Lacunarity);
					_2D_Perlin_Noise_Fractal_2_Noise.SetFractalType(EVoxelNoiseFractalType::FBM);
					
					// Set octaves for all LOD levels
					for (int32 i = 0; i < 32; i++)
					{
						_2D_Perlin_Noise_Fractal_2_LODToOctaves[i] = Params.Base_Octaves;
					}
				}
				
				////////////////////////////////////////////////////
				//////////////////// Other inits ///////////////////
				////////////////////////////////////////////////////
				Function0_XYZWithoutCache_Init(InitStruct);
			}
			
			////////////////////////////////////////////////////
			//////////////// Compute constants /////////////////
			////////////////////////////////////////////////////
			{
				// 2D Perlin Noise Fractal
				TVoxelRange<v_flt> Variable_1; // 2D Perlin Noise Fractal output 0
				Variable_1 = { -0.601283f, 0.600269f };
				
				// 2D Noise SDF.* - NOW USING HEIGHT SCALE PARAMETER
				TVoxelRange<v_flt> Variable_5; // 2D Noise SDF.* output 0
				Variable_5 = Variable_1 * TVoxelRange<v_flt>(Params.Terrain_Height_Scale);
				
				// 2D Noise SDF.+
				BufferConstant.Variable_0 = Variable_5 + TVoxelRange<v_flt>(0.0f);
			}
		}
		void ComputeXYZWithoutCache(const FVoxelContextRange& Context, FOutputs& Outputs) const
		{
			Function0_XYZWithoutCache_Compute(Context, Outputs);
		}
		
		inline FBufferX GetBufferX() const { return {}; }
		inline FBufferXY GetBufferXY() const { return {}; }
		inline FOutputs GetOutputs() const { return {}; }
		
	private:
		FBufferConstant BufferConstant;
		
		const FParams& Params;
		
		FVoxelFastNoise _2D_Perlin_Noise_Fractal_2_Noise;
		TStaticArray<uint8, 32> _2D_Perlin_Noise_Fractal_2_LODToOctaves;
		
		///////////////////////////////////////////////////////////////////////
		//////////////////////////// Init functions ///////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
		}
		
		///////////////////////////////////////////////////////////////////////
		////////////////////////// Compute functions //////////////////////////
		///////////////////////////////////////////////////////////////////////
		
		void Function0_XYZWithoutCache_Compute(const FVoxelContextRange& Context, FOutputs& Outputs) const
		{
			// Z
			TVoxelRange<v_flt> Variable_2; // Z output 0
			Variable_2 = Context.GetLocalZ();
			
			// 2D Noise SDF.-
			TVoxelRange<v_flt> Variable_4; // 2D Noise SDF.- output 0
			Variable_4 = Variable_2 - BufferConstant.Variable_0;
			
			// Set High Quality Value.*
			TVoxelRange<v_flt> Variable_3; // Set High Quality Value.* output 0
			Variable_3 = Variable_4 * TVoxelRange<v_flt>(0.2f);
			
			Outputs.Value = Variable_3;
		}
	};
	
	FVG_Alfheim(UVG_Alfheim& Object)
			: TVoxelGraphGeneratorInstanceHelper(
			{
				{ "Value", 1 },
			},
			{
			},
			{
			},
			{
				{
					{ "Value", NoTransformAccessor<v_flt>::Get<1, TOutputFunctionPtr<v_flt>>() },
				},
				{
				},
				{
				},
				{
					{ "Value", NoTransformRangeAccessor<v_flt>::Get<1, TRangeOutputFunctionPtr<v_flt>>() },
				}
			},
			{
				{
					{ "Value", WithTransformAccessor<v_flt>::Get<1, TOutputFunctionPtr_Transform<v_flt>>() },
				},
				{
				},
				{
				},
				{
					{ "Value", WithTransformRangeAccessor<v_flt>::Get<1, TRangeOutputFunctionPtr_Transform<v_flt>>() },
				}
			},
			Object)
		, Params(FParams
		{
			*Object.Layer_0.GetAssetName(),
			*Object.Layer_1.GetAssetName(),
			*Object.Layer_2.GetAssetName(),
			*Object.Layer_3.GetAssetName(),
			Object.Master_Seed,
			Object.Terrain_Height_Scale,
			Object.Base_Frequency,
			Object.Base_Octaves,
			Object.Base_Gain,
			Object.Base_Lacunarity,
			Object.Material_Frequency,
			Object.Material_Octaves,
			Object.Material_Gain,
			Object.Material_Lacunarity,
			Object.Material_Variation_Strength,
			Object.Layer_1_Height,
			Object.Layer_1_Blend,
			Object.Layer_2_Height,
			Object.Layer_2_Blend,
			Object.Layer_3_Height,
			Object.Layer_3_Blend,
			Object.Enable_Island_Mode,
			Object.Island_Radius,
			Object.Island_Falloff_Distance,
			Object.Island_Falloff_Strength
		})
		, LocalValue(Params)
		, LocalMaterial(Params)
		, LocalUpVectorXUpVectorYUpVectorZ(Params)
		, LocalValueRangeAnalysis(Params)
	{
	}
	
	virtual void InitGraph(const FVoxelGeneratorInit& InitStruct) override final
	{
		LocalValue.Init(InitStruct);
		LocalMaterial.Init(InitStruct);
		LocalUpVectorXUpVectorYUpVectorZ.Init(InitStruct);
		LocalValueRangeAnalysis.Init(InitStruct);
	}
	
	template<uint32... Permutation>
	auto& GetTarget() const;
	
	template<uint32... Permutation>
	auto& GetRangeTarget() const;
	
private:
	FParams Params;
	FLocalComputeStruct_LocalValue LocalValue;
	FLocalComputeStruct_LocalMaterial LocalMaterial;
	FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ LocalUpVectorXUpVectorYUpVectorZ;
	FLocalComputeStruct_LocalValueRangeAnalysis LocalValueRangeAnalysis;
};

template<>
inline v_flt FVG_Alfheim::FLocalComputeStruct_LocalValue::FOutputs::Get<v_flt, 1>() const
{
	return Value;
}
template<>
inline void FVG_Alfheim::FLocalComputeStruct_LocalValue::FOutputs::Set<v_flt, 1>(v_flt InValue)
{
	Value = InValue;
}
template<>
inline FVoxelMaterial FVG_Alfheim::FLocalComputeStruct_LocalMaterial::FOutputs::Get<FVoxelMaterial, 2>() const
{
	return MaterialBuilder.Build();
}
template<>
inline void FVG_Alfheim::FLocalComputeStruct_LocalMaterial::FOutputs::Set<FVoxelMaterial, 2>(FVoxelMaterial Material)
{
}
template<>
inline v_flt FVG_Alfheim::FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ::FOutputs::Get<v_flt, 3>() const
{
	return UpVectorX;
}
template<>
inline void FVG_Alfheim::FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ::FOutputs::Set<v_flt, 3>(v_flt InValue)
{
	UpVectorX = InValue;
}
template<>
inline v_flt FVG_Alfheim::FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ::FOutputs::Get<v_flt, 4>() const
{
	return UpVectorY;
}
template<>
inline void FVG_Alfheim::FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ::FOutputs::Set<v_flt, 4>(v_flt InValue)
{
	UpVectorY = InValue;
}
template<>
inline v_flt FVG_Alfheim::FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ::FOutputs::Get<v_flt, 5>() const
{
	return UpVectorZ;
}
template<>
inline void FVG_Alfheim::FLocalComputeStruct_LocalUpVectorXUpVectorYUpVectorZ::FOutputs::Set<v_flt, 5>(v_flt InValue)
{
	UpVectorZ = InValue;
}
template<>
inline TVoxelRange<v_flt> FVG_Alfheim::FLocalComputeStruct_LocalValueRangeAnalysis::FOutputs::Get<v_flt, 1>() const
{
	return Value;
}
template<>
inline void FVG_Alfheim::FLocalComputeStruct_LocalValueRangeAnalysis::FOutputs::Set<v_flt, 1>(TVoxelRange<v_flt> InValue)
{
	Value = InValue;
}
template<>
inline auto& FVG_Alfheim::GetTarget<1>() const
{
	return LocalValue;
}
template<>
inline auto& FVG_Alfheim::GetTarget<2>() const
{
	return LocalMaterial;
}
template<>
inline auto& FVG_Alfheim::GetRangeTarget<0, 1>() const
{
	return LocalValueRangeAnalysis;
}
template<>
inline auto& FVG_Alfheim::GetTarget<3, 4, 5>() const
{
	return LocalUpVectorXUpVectorYUpVectorZ;
}
#endif

////////////////////////////////////////////////////////////
////////////////////////// UCLASS //////////////////////////
////////////////////////////////////////////////////////////

UVG_Alfheim::UVG_Alfheim()
{
	bEnableRangeAnalysis = true;
}

TVoxelSharedRef<FVoxelTransformableGeneratorInstance> UVG_Alfheim::GetTransformableInstance()
{
#if VOXEL_GRAPH_GENERATED_VERSION == 1
	return MakeVoxelShared<FVG_Alfheim>(*this);
#else
#if VOXEL_GRAPH_GENERATED_VERSION > 1
	EMIT_CUSTOM_WARNING("Outdated generated voxel graph: VG_Example_MultiIndex. You need to regenerate it.");
	FVoxelMessages::Warning("Outdated generated voxel graph: VG_Example_MultiIndex. You need to regenerate it.");
#else
	EMIT_CUSTOM_WARNING("Generated voxel graph is more recent than the Voxel Plugin version: VG_Example_MultiIndex. You need to update the plugin.");
	FVoxelMessages::Warning("Generated voxel graph is more recent than the Voxel Plugin version: VG_Example_MultiIndex. You need to update the plugin.");
#endif
	return MakeVoxelShared<FVoxelTransformableEmptyGeneratorInstance>();
#endif
}

PRAGMA_GENERATED_VOXEL_GRAPH_END