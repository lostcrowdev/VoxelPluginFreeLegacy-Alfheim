// Alfheim terrain generator - island with mountain and ocean terracing.

#include "VG_Alfheim.h"

PRAGMA_GENERATED_VOXEL_GRAPH_START

using FVoxelGraphSeed = int32;

#if VOXEL_GRAPH_GENERATED_VERSION == 1
class FVG_Alfheim : public TVoxelGraphGeneratorInstanceHelper<FVG_Alfheim, UVG_Alfheim>
{
public:
	struct FParams
	{
		const FName Layer0Material;
		const FName Layer1Material;
		const FName Layer2Material;
		const FName Layer3Material;
		const int32 Seed;
		const float HeightScale;
		const float NoiseFrequency;
		const int32 NoiseOctaves;
		const float NoiseGain;
		const float NoiseLacunarity;
		const float MountainHeightExponent;
		const float OceanDepthExponent;
		const float LandOceanBalance;
		const bool bEnableIslandMode;
		const float IslandRadius;
		const float IslandFalloffDistance;
		const float IslandFalloffStrength;
		const bool bEnableBeachPlateau;
		const float BeachHeight;
		const float BeachWidth;
		const float BeachBlendSharpness;
		const FVoxelTerraceSettings MountainTerraces;
		const FVoxelTerraceSettings OceanTerraces;
		const bool bEnableOceanFloor;
		const float OceanFloorDepth;
		const float OceanFloorRampDistance;
		const float Layer1Height;
		const float Layer1Blend;
		const float Layer2Height;
		const float Layer2Blend;
		const float Layer3Height;
		const float Layer3Blend;
	};

	// Figures out how high the noise can actually stack up given the octaves/gain, so the
	// clamp and range analysis stay accurate no matter how you tune those.
	static float EstimateFractalAmplitudeBound(int32 Octaves, float Gain)
	{
		const float ClampedGain = FMath::Clamp(Gain, 0.01f, 0.999f);
		float Sum = 0.0f;
		float Amplitude = 1.0f;
		for (int32 i = 0; i < Octaves; i++)
		{
			Sum += Amplitude;
			Amplitude *= ClampedGain;
		}
		return Sum;
	}

	// Pushes normalized noise toward or away from 0 differently on the land side vs the
	// ocean side, without changing its actual min/max range.
	static v_flt ApplyHeightExponentBias(v_flt NormalizedNoise, float MountainExponent, float OceanExponent)
	{
		if (NormalizedNoise >= v_flt(0.0f))
		{
			return FMath::Pow(NormalizedNoise, v_flt(MountainExponent));
		}
		else
		{
			return -FMath::Pow(-NormalizedNoise, v_flt(OceanExponent));
		}
	}

	// Nudges terrain toward a stack of flat shelves stepping away from BaseHeight in one
	// direction. Direction = +1 steps up (mountains), -1 steps down (ocean).
	static v_flt ApplyTerraceSteps(v_flt SDF, v_flt TerrainHeight, const FVoxelTerraceSettings& Settings, float Direction)
	{
		if (Settings.StepCount <= 0)
		{
			return SDF;
		}

		float StepTarget = Settings.BaseHeight;
		float CurrentShelfWidth = Settings.ShelfWidth;
		float CurrentIncrement = Settings.HeightIncrement;

		for (int32 i = 0; i < Settings.StepCount; i++)
		{
			const bool bOnCorrectSide = (Direction > 0.0f)
				? (TerrainHeight > v_flt(StepTarget))
				: (TerrainHeight < v_flt(StepTarget));

			if (bOnCorrectSide)
			{
				const v_flt HeightDiff = FMath::Abs(TerrainHeight - v_flt(StepTarget));
				v_flt Blend = v_flt(1.0f) - FVoxelMathNodeFunctions::SmoothStep(v_flt(0.0f), v_flt(CurrentShelfWidth), HeightDiff);
				Blend = FMath::Pow(Blend, v_flt(Settings.BlendSharpness));
				SDF = SDF + Blend * (TerrainHeight - v_flt(StepTarget));
			}

			StepTarget += Direction * CurrentIncrement;
			CurrentIncrement *= Settings.IncrementGrowth;
			CurrentShelfWidth *= Settings.ShelfWidthGrowth;
		}

		return SDF;
	}

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

			v_flt Variable_2;
		};

		struct FBufferXY
		{
			FBufferXY() {}

			v_flt Variable_0;
		};

		FLocalComputeStruct_LocalValue(const FParams& InParams)
			: Params(InParams)
		{
		}

		void Init(const FVoxelGeneratorInit& InitStruct)
		{
			{
				{
					FVoxelGraphSeed Variable_5;
					FVoxelGraphSeed Make_Seeds_0_Temp_1;
					Variable_5 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Seed));
					Make_Seeds_0_Temp_1 = FVoxelUtilities::MurmurHash32(Variable_5);
				}

				Function0_XYZWithoutCache_Init(InitStruct);
			}

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

		float BaseNoiseAmplitudeBound = 1.0f;

		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
			FVoxelGraphSeed Variable_5;
			FVoxelGraphSeed Make_Seeds_0_Temp_1;
			Variable_5 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Seed));
			Make_Seeds_0_Temp_1 = FVoxelUtilities::MurmurHash32(Variable_5);

			_2D_Perlin_Noise_Fractal_0_Noise.SetSeed(Variable_5);
			_2D_Perlin_Noise_Fractal_0_Noise.SetInterpolation(EVoxelNoiseInterpolation::Quintic);
			_2D_Perlin_Noise_Fractal_0_Noise.SetFractalOctavesAndGain(Params.NoiseOctaves, Params.NoiseGain);
			_2D_Perlin_Noise_Fractal_0_Noise.SetFractalLacunarity(Params.NoiseLacunarity);
			_2D_Perlin_Noise_Fractal_0_Noise.SetFractalType(EVoxelNoiseFractalType::FBM);

			for (int32 i = 0; i < 32; i++)
			{
				_2D_Perlin_Noise_Fractal_0_LODToOctaves[i] = Params.NoiseOctaves;
			}

			BaseNoiseAmplitudeBound = EstimateFractalAmplitudeBound(Params.NoiseOctaves, Params.NoiseGain);
		}

		void Function0_X_Compute(const FVoxelContext& Context, FBufferX& BufferX) const
		{
			BufferX.Variable_2 = Context.GetLocalX();
		}

		void Function0_XYWithCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			v_flt Variable_3;
			Variable_3 = Context.GetLocalY();

			v_flt Variable_1;
			Variable_1 = _2D_Perlin_Noise_Fractal_0_Noise.GetPerlinFractal_2D(BufferX.Variable_2, Variable_3, v_flt(Params.NoiseFrequency), _2D_Perlin_Noise_Fractal_0_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_1 = FMath::Clamp<v_flt>(Variable_1, -BaseNoiseAmplitudeBound, BaseNoiseAmplitudeBound);
			Variable_1 = ApplyHeightExponentBias(Variable_1 / v_flt(BaseNoiseAmplitudeBound), Params.MountainHeightExponent, Params.OceanDepthExponent) * v_flt(BaseNoiseAmplitudeBound);

			v_flt Variable_8;
			Variable_8 = Variable_1 * v_flt(Params.HeightScale);

			BufferXY.Variable_0 = Variable_8 + v_flt(Params.LandOceanBalance);
		}

		void Function0_XYWithoutCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			v_flt Variable_3;
			Variable_3 = Context.GetLocalY();

			BufferX.Variable_2 = Context.GetLocalX();

			v_flt Variable_1;
			Variable_1 = _2D_Perlin_Noise_Fractal_0_Noise.GetPerlinFractal_2D(BufferX.Variable_2, Variable_3, v_flt(Params.NoiseFrequency), _2D_Perlin_Noise_Fractal_0_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_1 = FMath::Clamp<v_flt>(Variable_1, -BaseNoiseAmplitudeBound, BaseNoiseAmplitudeBound);
			Variable_1 = ApplyHeightExponentBias(Variable_1 / v_flt(BaseNoiseAmplitudeBound), Params.MountainHeightExponent, Params.OceanDepthExponent) * v_flt(BaseNoiseAmplitudeBound);

			v_flt Variable_8;
			Variable_8 = Variable_1 * v_flt(Params.HeightScale);

			BufferXY.Variable_0 = Variable_8 + v_flt(Params.LandOceanBalance);
		}

		// Runs everything that reshapes terrain on top of the raw noise: beach, mountain and
		// ocean terracing, the guaranteed ocean floor, then the island edge falloff.
		v_flt ApplyTerrainShaping(v_flt SDF, v_flt TerrainHeight, v_flt X, v_flt Y) const
		{
			if (Params.bEnableBeachPlateau)
			{
				const v_flt HeightDiff = FMath::Abs(TerrainHeight - v_flt(Params.BeachHeight));
				v_flt BeachBlend = v_flt(1.0f) - FVoxelMathNodeFunctions::SmoothStep(v_flt(0.0f), v_flt(Params.BeachWidth), HeightDiff);
				BeachBlend = FMath::Pow(BeachBlend, v_flt(Params.BeachBlendSharpness));
				SDF = SDF + BeachBlend * (TerrainHeight - v_flt(Params.BeachHeight));
			}

			SDF = ApplyTerraceSteps(SDF, TerrainHeight, Params.MountainTerraces, 1.0f);

			SDF = ApplyTerraceSteps(SDF, TerrainHeight, Params.OceanTerraces, -1.0f);

			// Guarantees the sea floor reaches OceanFloorDepth past the shallow shelf, instead
			// of just relying on the terrace shelves nudging terrain that's already close.
			if (Params.bEnableOceanFloor && Params.OceanTerraces.StepCount > 0)
			{
				const v_flt ShallowShelfHeight = v_flt(Params.OceanTerraces.BaseHeight);
				if (TerrainHeight < ShallowShelfHeight)
				{
					const v_flt DepthPastShelf = ShallowShelfHeight - TerrainHeight;
					const v_flt PullStrength = FMath::Clamp<v_flt>(
						DepthPastShelf / v_flt(FMath::Max(1.0f, Params.OceanFloorRampDistance)), 0.0f, 1.0f);
					SDF = SDF + PullStrength * (TerrainHeight - v_flt(Params.OceanFloorDepth));
				}
			}

			// Carves terrain away past the island radius so it reads as one landmass.
			if (Params.bEnableIslandMode)
			{
				const v_flt DistanceFromCenter = FMath::Sqrt(X * X + Y * Y);

				const v_flt FalloffStart = v_flt(Params.IslandRadius);
				const v_flt FalloffEnd = v_flt(Params.IslandRadius + Params.IslandFalloffDistance);

				v_flt FalloffAmount = FVoxelMathNodeFunctions::SmoothStep(FalloffStart, FalloffEnd, DistanceFromCenter);
				FalloffAmount = FMath::Pow(FalloffAmount, v_flt(Params.IslandFalloffStrength));

				const v_flt HeightReduction = FalloffAmount * v_flt(Params.HeightScale * 2.0f);
				SDF = SDF + HeightReduction;
			}

			return SDF;
		}

		void Function0_XYZWithCache_Compute(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
			v_flt Variable_4;
			Variable_4 = Context.GetLocalZ();

			v_flt Variable_7;
			Variable_7 = Variable_4 - BufferXY.Variable_0;

			Variable_7 = ApplyTerrainShaping(Variable_7, BufferXY.Variable_0, BufferX.Variable_2, Context.GetLocalY());

			v_flt Variable_6;
			Variable_6 = Variable_7 * v_flt(0.2f);

			Outputs.Value = Variable_6;
		}

		void Function0_XYZWithoutCache_Compute(const FVoxelContext& Context, FOutputs& Outputs) const
		{
			v_flt Variable_4;
			Variable_4 = Context.GetLocalZ();

			v_flt Variable_3;
			Variable_3 = Context.GetLocalY();

			v_flt Variable_2;
			Variable_2 = Context.GetLocalX();

			v_flt Variable_1;
			Variable_1 = _2D_Perlin_Noise_Fractal_0_Noise.GetPerlinFractal_2D(Variable_2, Variable_3, v_flt(Params.NoiseFrequency), _2D_Perlin_Noise_Fractal_0_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_1 = FMath::Clamp<v_flt>(Variable_1, -BaseNoiseAmplitudeBound, BaseNoiseAmplitudeBound);
			Variable_1 = ApplyHeightExponentBias(Variable_1 / v_flt(BaseNoiseAmplitudeBound), Params.MountainHeightExponent, Params.OceanDepthExponent) * v_flt(BaseNoiseAmplitudeBound);

			v_flt Variable_8;
			Variable_8 = Variable_1 * v_flt(Params.HeightScale);

			v_flt Variable_0;
			Variable_0 = Variable_8 + v_flt(Params.LandOceanBalance);

			v_flt Variable_7;
			Variable_7 = Variable_4 - Variable_0;

			Variable_7 = ApplyTerrainShaping(Variable_7, Variable_0, Variable_2, Variable_3);

			v_flt Variable_6;
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

			int32 Variable_6;
			int32 Variable_7;
			int32 Variable_8;
			int32 Variable_0;
		};

		struct FBufferX
		{
			FBufferX() {}

			v_flt Variable_11;
		};

		struct FBufferXY
		{
			FBufferXY() {}

			v_flt Variable_14;
		};

		FLocalComputeStruct_LocalMaterial(const FParams& InParams)
			: Params(InParams)
		{
		}

		void Init(const FVoxelGeneratorInit& InitStruct)
		{
			{
				{
					FVoxelGraphSeed Make_Seeds_1_Temp_0;
					FVoxelGraphSeed Variable_10;
					Make_Seeds_1_Temp_0 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Seed));
					Variable_10 = FVoxelUtilities::MurmurHash32(Make_Seeds_1_Temp_0);

					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_1_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer1Material);
					}
					else
					{
						Get_Material_Collection_Index__Layer_1_0_Index = -1;
					}

					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_2_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer2Material);
					}
					else
					{
						Get_Material_Collection_Index__Layer_2_0_Index = -1;
					}

					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_3_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer3Material);
					}
					else
					{
						Get_Material_Collection_Index__Layer_3_0_Index = -1;
					}

					if (InitStruct.MaterialCollection)
					{
						Get_Material_Collection_Index__Layer_0_0_Index = InitStruct.MaterialCollection->GetMaterialIndex(Params.Layer0Material);
					}
					else
					{
						Get_Material_Collection_Index__Layer_0_0_Index = -1;
					}

					UE_LOG(LogTemp, Warning, TEXT("[Alfheim] LocalMaterial::Init running. MaterialCollection=%s"),
						InitStruct.MaterialCollection ? *InitStruct.MaterialCollection->GetName() : TEXT("NULL"));
					UE_LOG(LogTemp, Warning, TEXT("[Alfheim] Layer0Material='%s' -> Index=%d"), *Params.Layer0Material.ToString(), Get_Material_Collection_Index__Layer_0_0_Index);
					UE_LOG(LogTemp, Warning, TEXT("[Alfheim] Layer1Material='%s' -> Index=%d"), *Params.Layer1Material.ToString(), Get_Material_Collection_Index__Layer_1_0_Index);
					UE_LOG(LogTemp, Warning, TEXT("[Alfheim] Layer2Material='%s' -> Index=%d"), *Params.Layer2Material.ToString(), Get_Material_Collection_Index__Layer_2_0_Index);
					UE_LOG(LogTemp, Warning, TEXT("[Alfheim] Layer3Material='%s' -> Index=%d"), *Params.Layer3Material.ToString(), Get_Material_Collection_Index__Layer_3_0_Index);
					if (Get_Material_Collection_Index__Layer_0_0_Index == -1 || Get_Material_Collection_Index__Layer_1_0_Index == -1 ||
						Get_Material_Collection_Index__Layer_2_0_Index == -1 || Get_Material_Collection_Index__Layer_3_0_Index == -1)
					{
						UE_LOG(LogTemp, Error, TEXT("[Alfheim] One or more layers failed to resolve against the Material Collection (-1) - that layer's name doesn't exist in the collection assigned to this Voxel World."));
					}
				}

				Function0_XYZWithoutCache_Init(InitStruct);
			}

			{
				BufferConstant.Variable_6 = Get_Material_Collection_Index__Layer_1_0_Index;

				BufferConstant.Variable_7 = Get_Material_Collection_Index__Layer_2_0_Index;

				BufferConstant.Variable_8 = Get_Material_Collection_Index__Layer_3_0_Index;

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

		// The material wobble noise and the layer height thresholds/blends live on the
		// UVG_Alfheim properties instead (see Params.Layer1Height etc). These below are just
		// the wobble noise's shape - edit these directly if you want to retune it, they're
		// not worth exposing since you'd rarely touch them.
		static constexpr int32 MaterialNoiseOctaves = 8;
		static constexpr float MaterialNoiseGain = 0.708684f;
		static constexpr float MaterialNoiseLacunarity = 2.0f;
		static constexpr float MaterialNoiseFrequency = 0.002853f;
		static constexpr float MaterialVariationStrength = 13.469243f;

		float MaterialNoiseAmplitudeBound = 1.0f;

		mutable FThreadSafeCounter DebugWeightLogCounterWithCache;
		mutable FThreadSafeCounter DebugWeightLogCounterWithoutCache;

		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
			FVoxelGraphSeed Make_Seeds_1_Temp_0;
			FVoxelGraphSeed Variable_10;
			Make_Seeds_1_Temp_0 = FVoxelUtilities::MurmurHash32(FVoxelGraphSeed(Params.Seed));
			Variable_10 = FVoxelUtilities::MurmurHash32(Make_Seeds_1_Temp_0);

			_2D_Perlin_Noise_Fractal_1_Noise.SetSeed(Variable_10);
			_2D_Perlin_Noise_Fractal_1_Noise.SetInterpolation(EVoxelNoiseInterpolation::Quintic);
			_2D_Perlin_Noise_Fractal_1_Noise.SetFractalOctavesAndGain(MaterialNoiseOctaves, MaterialNoiseGain);
			_2D_Perlin_Noise_Fractal_1_Noise.SetFractalLacunarity(MaterialNoiseLacunarity);
			_2D_Perlin_Noise_Fractal_1_Noise.SetFractalType(EVoxelNoiseFractalType::FBM);

			for (int32 i = 0; i < 32; i++)
			{
				_2D_Perlin_Noise_Fractal_1_LODToOctaves[i] = MaterialNoiseOctaves;
			}

			MaterialNoiseAmplitudeBound = EstimateFractalAmplitudeBound(MaterialNoiseOctaves, MaterialNoiseGain);
		}

		void Function0_X_Compute(const FVoxelContext& Context, FBufferX& BufferX) const
		{
			BufferX.Variable_11 = Context.GetLocalX();
		}

		void Function0_XYWithCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			v_flt Variable_12;
			Variable_12 = Context.GetLocalY();

			v_flt Variable_13;
			Variable_13 = _2D_Perlin_Noise_Fractal_1_Noise.GetPerlinFractal_2D(BufferX.Variable_11, Variable_12, v_flt(MaterialNoiseFrequency), _2D_Perlin_Noise_Fractal_1_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_13 = FMath::Clamp<v_flt>(Variable_13, -MaterialNoiseAmplitudeBound, MaterialNoiseAmplitudeBound);

			BufferXY.Variable_14 = Variable_13 * v_flt(MaterialVariationStrength);
		}

		void Function0_XYWithoutCache_Compute(const FVoxelContext& Context, FBufferX& BufferX, FBufferXY& BufferXY) const
		{
			v_flt Variable_12;
			Variable_12 = Context.GetLocalY();

			BufferX.Variable_11 = Context.GetLocalX();

			v_flt Variable_13;
			Variable_13 = _2D_Perlin_Noise_Fractal_1_Noise.GetPerlinFractal_2D(BufferX.Variable_11, Variable_12, v_flt(MaterialNoiseFrequency), _2D_Perlin_Noise_Fractal_1_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_13 = FMath::Clamp<v_flt>(Variable_13, -MaterialNoiseAmplitudeBound, MaterialNoiseAmplitudeBound);

			BufferXY.Variable_14 = Variable_13 * v_flt(MaterialVariationStrength);
		}

		void Function0_XYZWithCache_Compute(const FVoxelContext& Context, const FBufferX& BufferX, const FBufferXY& BufferXY, FOutputs& Outputs) const
		{
			v_flt Variable_5;
			Variable_5 = Context.GetLocalZ();

			v_flt Variable_9;
			Variable_9 = Variable_5 + BufferXY.Variable_14;

			v_flt Variable_1;
			v_flt Variable_2;
			v_flt Variable_3;
			v_flt Variable_4;
			{
				TVoxelStaticArray<v_flt, 6> InputsArray;
				TVoxelStaticArray<v_flt, 4> OutputsArray;
				InputsArray[0] = v_flt(Params.Layer1Height);
				InputsArray[1] = v_flt(Params.Layer1Blend);
				InputsArray[2] = v_flt(Params.Layer2Height);
				InputsArray[3] = v_flt(Params.Layer2Blend);
				InputsArray[4] = v_flt(Params.Layer3Height);
				InputsArray[5] = v_flt(Params.Layer3Blend);
				FVoxelMathNodeFunctions::HeightSplit(Variable_9, InputsArray, OutputsArray);
				Variable_1 = OutputsArray[0];
				Variable_2 = OutputsArray[1];
				Variable_3 = OutputsArray[2];
				Variable_4 = OutputsArray[3];
			}

			{
				const v_flt WeightSum = Variable_1 + Variable_2 + Variable_3 + Variable_4;
				if (WeightSum > v_flt(0.0001))
				{
					Variable_1 = Variable_1 / WeightSum;
					Variable_2 = Variable_2 / WeightSum;
					Variable_3 = Variable_3 / WeightSum;
					Variable_4 = Variable_4 / WeightSum;
				}
			}

			if (DebugWeightLogCounterWithCache.GetValue() < 40)
			{
				const int32 LogIndex = DebugWeightLogCounterWithCache.Increment();
				if (LogIndex <= 40)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Alfheim] Weight sample #%d (WithCache): HeightInput=%.2f Weights[idx%d=%.3f idx%d=%.3f idx%d=%.3f idx%d=%.3f]"),
						LogIndex, (double)Variable_9,
						BufferConstant.Variable_0, (double)Variable_1,
						BufferConstant.Variable_6, (double)Variable_2,
						BufferConstant.Variable_7, (double)Variable_3,
						BufferConstant.Variable_8, (double)Variable_4);
				}
			}

			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_0, Variable_1, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_6, Variable_2, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_7, Variable_3, bool(false));
			Outputs.MaterialBuilder.AddMultiIndex(BufferConstant.Variable_8, Variable_4, bool(false));
		}

		void Function0_XYZWithoutCache_Compute(const FVoxelContext& Context, FOutputs& Outputs) const
		{
			v_flt Variable_12;
			Variable_12 = Context.GetLocalY();

			v_flt Variable_5;
			Variable_5 = Context.GetLocalZ();

			v_flt Variable_11;
			Variable_11 = Context.GetLocalX();

			v_flt Variable_13;
			Variable_13 = _2D_Perlin_Noise_Fractal_1_Noise.GetPerlinFractal_2D(Variable_11, Variable_12, v_flt(MaterialNoiseFrequency), _2D_Perlin_Noise_Fractal_1_LODToOctaves[FMath::Clamp(Context.LOD, 0, 31)]);
			Variable_13 = FMath::Clamp<v_flt>(Variable_13, -MaterialNoiseAmplitudeBound, MaterialNoiseAmplitudeBound);

			v_flt Variable_14;
			Variable_14 = Variable_13 * v_flt(MaterialVariationStrength);

			v_flt Variable_9;
			Variable_9 = Variable_5 + Variable_14;

			v_flt Variable_1;
			v_flt Variable_2;
			v_flt Variable_3;
			v_flt Variable_4;
			{
				TVoxelStaticArray<v_flt, 6> InputsArray;
				TVoxelStaticArray<v_flt, 4> OutputsArray;
				InputsArray[0] = v_flt(Params.Layer1Height);
				InputsArray[1] = v_flt(Params.Layer1Blend);
				InputsArray[2] = v_flt(Params.Layer2Height);
				InputsArray[3] = v_flt(Params.Layer2Blend);
				InputsArray[4] = v_flt(Params.Layer3Height);
				InputsArray[5] = v_flt(Params.Layer3Blend);
				FVoxelMathNodeFunctions::HeightSplit(Variable_9, InputsArray, OutputsArray);
				Variable_1 = OutputsArray[0];
				Variable_2 = OutputsArray[1];
				Variable_3 = OutputsArray[2];
				Variable_4 = OutputsArray[3];
			}

			{
				const v_flt WeightSum = Variable_1 + Variable_2 + Variable_3 + Variable_4;
				if (WeightSum > v_flt(0.0001))
				{
					Variable_1 = Variable_1 / WeightSum;
					Variable_2 = Variable_2 / WeightSum;
					Variable_3 = Variable_3 / WeightSum;
					Variable_4 = Variable_4 / WeightSum;
				}
			}

			if (DebugWeightLogCounterWithoutCache.GetValue() < 40)
			{
				const int32 LogIndex = DebugWeightLogCounterWithoutCache.Increment();
				if (LogIndex <= 40)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Alfheim] Weight sample #%d (WithoutCache): HeightInput=%.2f Weights[idx%d=%.3f idx%d=%.3f idx%d=%.3f idx%d=%.3f]"),
						LogIndex, (double)Variable_9,
						BufferConstant.Variable_0, (double)Variable_1,
						BufferConstant.Variable_6, (double)Variable_2,
						BufferConstant.Variable_7, (double)Variable_3,
						BufferConstant.Variable_8, (double)Variable_4);
				}
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
			Function0_XYZWithoutCache_Init(InitStruct);
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

		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
		}

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

			TVoxelRange<v_flt> Variable_0;
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
			{
				{
					_2D_Perlin_Noise_Fractal_2_Noise.SetSeed(FVoxelGraphSeed(Params.Seed));
					_2D_Perlin_Noise_Fractal_2_Noise.SetInterpolation(EVoxelNoiseInterpolation::Quintic);
					_2D_Perlin_Noise_Fractal_2_Noise.SetFractalOctavesAndGain(Params.NoiseOctaves, Params.NoiseGain);
					_2D_Perlin_Noise_Fractal_2_Noise.SetFractalLacunarity(Params.NoiseLacunarity);
					_2D_Perlin_Noise_Fractal_2_Noise.SetFractalType(EVoxelNoiseFractalType::FBM);

					for (int32 i = 0; i < 32; i++)
					{
						_2D_Perlin_Noise_Fractal_2_LODToOctaves[i] = Params.NoiseOctaves;
					}
				}

				Function0_XYZWithoutCache_Init(InitStruct);
			}

			{
				const float AmplitudeBound = EstimateFractalAmplitudeBound(Params.NoiseOctaves, Params.NoiseGain);
				TVoxelRange<v_flt> Variable_1;
				Variable_1 = { -AmplitudeBound, AmplitudeBound };

				TVoxelRange<v_flt> Variable_5;
				Variable_5 = Variable_1 * TVoxelRange<v_flt>(Params.HeightScale);

				BufferConstant.Variable_0 = Variable_5 + TVoxelRange<v_flt>(Params.LandOceanBalance);
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

		void Function0_XYZWithoutCache_Init(const FVoxelGeneratorInit& InitStruct)
		{
		}

		void Function0_XYZWithoutCache_Compute(const FVoxelContextRange& Context, FOutputs& Outputs) const
		{
			TVoxelRange<v_flt> Variable_2;
			Variable_2 = Context.GetLocalZ();

			TVoxelRange<v_flt> Variable_4;
			Variable_4 = Variable_2 - BufferConstant.Variable_0;

			// Widens the octree's expected height range for anything that pushes terrain
			// further than the base noise bound alone would predict, so nothing gets culled
			// or clipped incorrectly. Beach nudges both directions, island falloff only pushes
			// up (carves away), ocean floor pull only pushes down.
			float LowerPad = 0.0f;
			float UpperPad = 0.0f;

			if (Params.bEnableBeachPlateau)
			{
				LowerPad = FMath::Max(LowerPad, Params.BeachWidth);
				UpperPad = FMath::Max(UpperPad, Params.BeachWidth);
			}
			if (Params.bEnableIslandMode)
			{
				UpperPad += Params.HeightScale * 2.0f;
			}
			if (Params.bEnableOceanFloor)
			{
				LowerPad = FMath::Max(LowerPad, FMath::Abs(Params.OceanFloorDepth));
			}

			TVoxelRange<v_flt> Padding;
			Padding = { -LowerPad, UpperPad };
			Variable_4 = Variable_4 + Padding;

			TVoxelRange<v_flt> Variable_3;
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
			*Object.Layer0Material.GetAssetName(),
			*Object.Layer1Material.GetAssetName(),
			*Object.Layer2Material.GetAssetName(),
			*Object.Layer3Material.GetAssetName(),
			Object.Seed,
			Object.HeightScale,
			Object.NoiseFrequency,
			Object.NoiseOctaves,
			Object.NoiseGain,
			Object.NoiseLacunarity,
			Object.MountainHeightExponent,
			Object.OceanDepthExponent,
			Object.LandOceanBalance,
			Object.bEnableIslandMode,
			Object.IslandRadius,
			Object.IslandFalloffDistance,
			Object.IslandFalloffStrength,
			Object.bEnableBeachPlateau,
			Object.BeachHeight,
			Object.BeachWidth,
			Object.BeachBlendSharpness,
			Object.MountainTerraces,
			Object.OceanTerraces,
			Object.bEnableOceanFloor,
			Object.OceanFloorDepth,
			Object.OceanFloorRampDistance,
			Object.Layer1Height,
			Object.Layer1Blend,
			Object.Layer2Height,
			Object.Layer2Blend,
			Object.Layer3Height,
			Object.Layer3Blend
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

UVG_Alfheim::UVG_Alfheim()
{
	bEnableRangeAnalysis = true;

	// Single mountain cap.
	MountainTerraces.StepCount = 1;
	MountainTerraces.BaseHeight = 32.0f;
	MountainTerraces.ShelfWidth = 40.0f;
	MountainTerraces.BlendSharpness = 3.0f;

	// Shallow swimmable shelf, then a deeper shelf below it.
	OceanTerraces.StepCount = 2;
	OceanTerraces.BaseHeight = -40.0f;
	OceanTerraces.HeightIncrement = 45.0f;
	OceanTerraces.IncrementGrowth = 1.3f;
	OceanTerraces.ShelfWidth = 15.0f;
	OceanTerraces.ShelfWidthGrowth = 1.4f;
	OceanTerraces.BlendSharpness = 2.5f;
}

TVoxelSharedRef<FVoxelTransformableGeneratorInstance> UVG_Alfheim::GetTransformableInstance()
{
#if VOXEL_GRAPH_GENERATED_VERSION == 1
	return MakeVoxelShared<FVG_Alfheim>(*this);
#else
#if VOXEL_GRAPH_GENERATED_VERSION > 1
	EMIT_CUSTOM_WARNING("Outdated generated voxel graph: VG_Alfheim. You need to regenerate it.");
	FVoxelMessages::Warning("Outdated generated voxel graph: VG_Alfheim. You need to regenerate it.");
#else
	EMIT_CUSTOM_WARNING("Generated voxel graph is more recent than the Voxel Plugin version: VG_Alfheim. You need to update the plugin.");
	FVoxelMessages::Warning("Generated voxel graph is more recent than the Voxel Plugin version: VG_Alfheim. You need to update the plugin.");
#endif
	return MakeVoxelShared<FVoxelTransformableEmptyGeneratorInstance>();
#endif
}

PRAGMA_GENERATED_VOXEL_GRAPH_END
