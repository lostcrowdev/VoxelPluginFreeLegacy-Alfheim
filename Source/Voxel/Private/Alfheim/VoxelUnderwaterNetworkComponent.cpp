// VoxelUnderwaterNetworkComponent.cpp

#include "Alfheim/VoxelUnderwaterNetworkComponent.h"
#include "Alfheim/VoxelSplineNetworkGenerator.h"
#include "DrawDebugHelpers.h"

using FDelaunayTriangle = FVoxelDelaunayVoronoiUtils::FDelaunayTriangle;

UVoxelUnderwaterNetworkComponent::UVoxelUnderwaterNetworkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AVoxelSplineNetworkGenerator* UVoxelUnderwaterNetworkComponent::GetGenerator() const
{
	return GetOwner<AVoxelSplineNetworkGenerator>();
}

void UVoxelUnderwaterNetworkComponent::Clear()
{
	UnderwaterCells.Empty();
	CachedUnderwaterTriangles.Empty();
	CachedUnderwaterBiomeGrid.Empty();
	UnderwaterBiomeRowNames.Empty();    UnderwaterBiomeRows.Empty();
	UnderwaterLocationRowNames.Empty(); UnderwaterLocationRows.Empty();

	for (USplineComponent* S : UnderwaterBiomeSplines)  if (S) S->DestroyComponent();
	for (USplineComponent* S : UnderwaterCentreSplines) if (S) S->DestroyComponent();
	UnderwaterBiomeSplines.Empty(); UnderwaterCentreSplines.Empty();

	ClearLabelComponents();
}

void UVoxelUnderwaterNetworkComponent::RunPipeline()
{
	LoadUnderwaterDataTables();
	if (UnderwaterBiomeRows.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelUnderwaterNetworkComponent: UnderwaterBiomeDataTable is null or empty — skipping underwater network."));
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("VoxelUnderwaterNetworkComponent: Generating underwater network — %d biomes, %d locations."),
		UnderwaterBiomeRows.Num(), UnderwaterLocationRows.Num());

	GenerateUnderwaterCellPoints();
	BuildUnderwaterDelaunayGraph();
	ComputeUnderwaterVoronoiPolygons();
	TraceUnderwaterDepths();
	AssignUnderwaterBiomes();
	AssignUnderwaterLocations();
	RasterizeUnderwaterBiomeGrid();
	BuildUnderwaterCellSplines();
	BuildUnderwaterCentreSplines();

	int32 UWCount = 0;
	for (const FVoxelUnderwaterCell& C : UnderwaterCells) if (C.bUnderwater) ++UWCount;
	UE_LOG(LogTemp, Log,
		TEXT("VoxelUnderwaterNetworkComponent: Underwater done. %d cells (%d valid), "
		     "%d biome splines, %d centre splines."),
		UnderwaterCells.Num(), UWCount, UnderwaterBiomeSplines.Num(), UnderwaterCentreSplines.Num());
}

void UVoxelUnderwaterNetworkComponent::RefreshDebugDraw()
{
	const AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	ClearLabelComponents();
	if (Gen->bShowUnderwaterEdges)  DrawDebugEdges();
	if (Gen->bShowUnderwaterLabels) BuildLabelComponents();
}

void UVoxelUnderwaterNetworkComponent::LoadUnderwaterDataTables()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	UnderwaterBiomeRowNames.Empty(); UnderwaterBiomeRows.Empty();
	UnderwaterLocationRowNames.Empty(); UnderwaterLocationRows.Empty();
	
	if (Gen->UnderwaterBiomeDataTable)
	{
		for (const FName& Key : Gen->UnderwaterBiomeDataTable->GetRowNames())
			if (FVoxelUnderwaterBiomeRow* R = Gen->UnderwaterBiomeDataTable->FindRow<FVoxelUnderwaterBiomeRow>(Key, TEXT("")))
				{ UnderwaterBiomeRowNames.Add(Key); UnderwaterBiomeRows.Add(*R); }
		
		if (UnderwaterBiomeRows.IsEmpty())
		{
			for (const FName& Key : Gen->UnderwaterBiomeDataTable->GetRowNames())
			{
				if (FVoxelBiomeRow* R = Gen->UnderwaterBiomeDataTable->FindRow<FVoxelBiomeRow>(Key, TEXT("")))
				{
					FVoxelUnderwaterBiomeRow UWRow;
					UWRow.BiomeName        = R->BiomeName;
					UWRow.PCGTag           = R->PCGTag;
					UWRow.DebugColor       = R->DebugColor;
					UWRow.CanBorderBiomes  = R->CanBorderBiomes;
					UnderwaterBiomeRowNames.Add(Key);
					UnderwaterBiomeRows.Add(UWRow);
				}
			}
			if (!UnderwaterBiomeRows.IsEmpty())
				UE_LOG(LogTemp, Log,
					TEXT("VoxelUnderwaterNetworkComponent: Promoted %d FVoxelBiomeRow(s) → underwater biome rows."),
					UnderwaterBiomeRows.Num());
		}
	}

	if (Gen->UnderwaterLocationDataTable)
	{
		for (const FName& Key : Gen->UnderwaterLocationDataTable->GetRowNames())
			if (FVoxelUnderwaterLocationRow* R = Gen->UnderwaterLocationDataTable->FindRow<FVoxelUnderwaterLocationRow>(Key, TEXT("")))
				{ UnderwaterLocationRowNames.Add(Key); UnderwaterLocationRows.Add(*R); }

		if (UnderwaterLocationRows.IsEmpty())
		{
			for (const FName& Key : Gen->UnderwaterLocationDataTable->GetRowNames())
			{
				if (FVoxelLocationRow* R = Gen->UnderwaterLocationDataTable->FindRow<FVoxelLocationRow>(Key, TEXT("")))
				{
					FVoxelUnderwaterLocationRow UWLoc;
					UWLoc.DisplayName          = R->DisplayName;
					UWLoc.PCGTag               = R->PCGTag;
					UWLoc.CategoryTag          = R->CategoryTag;
					UWLoc.AllowedBiomes        = R->AllowedBiomes;
					UWLoc.CentreRadius         = R->CentreRadius;
					UWLoc.MinSpacingFromOthers = R->MinSpacingFromOthers;
					UWLoc.DebugColor           = R->DebugColor;
					UnderwaterLocationRowNames.Add(Key);
					UnderwaterLocationRows.Add(UWLoc);
				}
			}
			if (!UnderwaterLocationRows.IsEmpty())
				UE_LOG(LogTemp, Log,
					TEXT("VoxelUnderwaterNetworkComponent: Promoted %d FVoxelLocationRow(s) → underwater location rows."),
					UnderwaterLocationRows.Num());
		}
	}

	if (Gen->bUseExampleDataIfMissing)
	{
		if (UnderwaterBiomeRows.IsEmpty())
		{
			LoadExampleUnderwaterBiomeRows(UnderwaterBiomeRowNames, UnderwaterBiomeRows);
			UE_LOG(LogTemp, Log,
				TEXT("VoxelUnderwaterNetworkComponent: UnderwaterBiomeDataTable missing — using %d example row(s)."),
				UnderwaterBiomeRows.Num());
		}
		if (UnderwaterLocationRows.IsEmpty())
		{
			LoadExampleUnderwaterLocationRows(UnderwaterLocationRowNames, UnderwaterLocationRows);
			UE_LOG(LogTemp, Log,
				TEXT("VoxelUnderwaterNetworkComponent: UnderwaterLocationDataTable missing — using %d example row(s)."),
				UnderwaterLocationRows.Num());
		}
	}
}

void UVoxelUnderwaterNetworkComponent::LoadExampleUnderwaterBiomeRows(
	TArray<FName>& OutNames, TArray<FVoxelUnderwaterBiomeRow>& OutRows) const
{
	OutNames.Empty(); OutRows.Empty();

	auto Add = [&](FName Name, const FString& DisplayName, FName PCGTag, FLinearColor Colour,
	               float MinDepth, float MaxDepth, std::initializer_list<FName> CanBorder)
	{
		FVoxelUnderwaterBiomeRow Row;
		Row.BiomeName        = DisplayName;
		Row.PCGTag           = PCGTag;
		Row.DebugColor       = Colour;
		Row.PreferredMinDepth = MinDepth;
		Row.PreferredMaxDepth = MaxDepth;
		Row.CanBorderBiomes  = CanBorder;
		OutNames.Add(Name); OutRows.Add(Row);
	};

	Add(TEXT("ShallowReef"),  TEXT("Shallow Reef"),  TEXT("PCG_UW_Biome_ShallowReef"),  FLinearColor(0.10f,0.55f,0.60f), 0.f,     20000.f,  {TEXT("KelpForest")});
	Add(TEXT("KelpForest"),   TEXT("Kelp Forest"),   TEXT("PCG_UW_Biome_KelpForest"),   FLinearColor(0.05f,0.35f,0.30f), 10000.f, 60000.f,  {TEXT("ShallowReef"),TEXT("AbyssalPlain")});
	Add(TEXT("AbyssalPlain"), TEXT("Abyssal Plain"), TEXT("PCG_UW_Biome_AbyssalPlain"), FLinearColor(0.02f,0.05f,0.15f), 50000.f, 500000.f, {TEXT("KelpForest")});
}

void UVoxelUnderwaterNetworkComponent::LoadExampleUnderwaterLocationRows(
	TArray<FName>& OutNames, TArray<FVoxelUnderwaterLocationRow>& OutRows) const
{
	OutNames.Empty(); OutRows.Empty();

	auto Add = [&](FName Name, const FString& DisplayName, FName PCGTag, FName Category,
	               std::initializer_list<FName> Allowed, float Radius, float Spacing,
	               float MinDepth, float MaxDepth)
	{
		FVoxelUnderwaterLocationRow Row;
		Row.DisplayName          = DisplayName;
		Row.PCGTag               = PCGTag;
		Row.CategoryTag          = Category;
		Row.AllowedBiomes        = Allowed;
		Row.CentreRadius         = Radius;
		Row.MinSpacingFromOthers = Spacing;
		Row.MinDepth             = MinDepth;
		Row.MaxDepth             = MaxDepth;
		OutNames.Add(Name); OutRows.Add(Row);
	};

	Add(TEXT("SunkenTemple"), TEXT("Sunken Temple"), TEXT("PCG_UW_Loc_SunkenTemple"), TEXT("PCG_UW_Ruin"),
		{TEXT("KelpForest")}, 8000.f, 60000.f, 10000.f, 60000.f);
	Add(TEXT("Shipwreck"), TEXT("Shipwreck"), TEXT("PCG_UW_Loc_Shipwreck"), TEXT("PCG_UW_Wreck"),
		{TEXT("ShallowReef")}, 5000.f, 40000.f, 0.f, 20000.f);
}

void UVoxelUnderwaterNetworkComponent::GenerateUnderwaterCellPoints()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	FRandomStream Rand(Gen->UnderwaterSeed);
	const int32 GridDim     = FMath::CeilToInt(FMath::Sqrt((float)Gen->UnderwaterCellCount));
	const float CellSpacing = Gen->GetComputedNetworkWidth() / FMath::Max(GridDim-1, 1);
	const float HalfWidth   = Gen->GetComputedNetworkWidth() * 0.5f;
	const float JitterMax   = CellSpacing * Gen->UnderwaterJitterAmount * 0.5f;
	const FVector Origin    = Gen->GetActorLocation();

	UnderwaterCells.Reserve(GridDim * GridDim);
	for (int32 Row=0; Row<GridDim; ++Row)
		for (int32 Col=0; Col<GridDim; ++Col)
		{
			FVoxelUnderwaterCell Cell;
			Cell.WorldPosition.X = Origin.X - HalfWidth + Col*CellSpacing + Rand.FRandRange(-JitterMax, JitterMax);
			Cell.WorldPosition.Y = Origin.Y - HalfWidth + Row*CellSpacing + Rand.FRandRange(-JitterMax, JitterMax);
			Cell.WorldPosition.Z = 0.f;
			UnderwaterCells.Add(Cell);
		}
}

void UVoxelUnderwaterNetworkComponent::BuildUnderwaterDelaunayGraph()
{
	if (UnderwaterCells.Num() < 3) return;
	TArray<FVector2D> Pts;
	Pts.Reserve(UnderwaterCells.Num());
	for (const FVoxelUnderwaterCell& C : UnderwaterCells)
		Pts.Add(FVector2D(C.WorldPosition.X, C.WorldPosition.Y));

	CachedUnderwaterTriangles = FVoxelDelaunayVoronoiUtils::BowyerWatsonFromPoints(Pts);
	for (const FDelaunayTriangle& T : CachedUnderwaterTriangles)
	{
		auto Link = [&](int32 A, int32 B)
		{ if (UnderwaterCells.IsValidIndex(A)&&UnderwaterCells.IsValidIndex(B)) UnderwaterCells[A].NeighborIndices.AddUnique(B); };
		Link(T.A,T.B); Link(T.B,T.A); Link(T.B,T.C); Link(T.C,T.B); Link(T.A,T.C); Link(T.C,T.A);
	}
}

void UVoxelUnderwaterNetworkComponent::ComputeUnderwaterVoronoiPolygons()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	TArray<FVector2D> Positions;
	Positions.Reserve(UnderwaterCells.Num());
	for (const FVoxelUnderwaterCell& C : UnderwaterCells)
		Positions.Add(FVector2D(C.WorldPosition.X, C.WorldPosition.Y));

	const FBox2D Bounds = Gen->GetNetworkBounds2D();
	for (int32 i=0; i<UnderwaterCells.Num(); ++i)
		UnderwaterCells[i].VoronoiPolygon2D =
			FVoxelDelaunayVoronoiUtils::ComputeVoronoiPolygon2D(i, Positions, CachedUnderwaterTriangles, Bounds);
}

void UVoxelUnderwaterNetworkComponent::TraceUnderwaterDepths()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	for (FVoxelUnderwaterCell& C : UnderwaterCells)
	{
		FVector Hit;
		if (Gen->TraceGroundAt(C.WorldPosition.X, C.WorldPosition.Y, Hit))
		{
			C.SeabedZ = Hit.Z;
			C.WorldPosition.Z = Hit.Z;
			C.bUnderwater = (Hit.Z < Gen->CoastHeightThreshold) && (Hit.Z > Gen->UnderwaterFloorZ);
		}
	}
}

void UVoxelUnderwaterNetworkComponent::AssignUnderwaterBiomes()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen || UnderwaterBiomeRows.IsEmpty()) return;

	FRandomStream Rand(Gen->UnderwaterSeed + 1);
	TArray<bool> Assigned; Assigned.Init(false, UnderwaterCells.Num());
	TQueue<int32> Q;

	int32 Start = -1;
	for (int32 i=0; i<UnderwaterCells.Num(); ++i)
		if (UnderwaterCells[i].bUnderwater) { Start=i; break; }
	if (Start < 0) return;

	UnderwaterCells[Start].BiomeRowName = UnderwaterBiomeRowNames[Rand.RandRange(0, UnderwaterBiomeRowNames.Num()-1)];
	Assigned[Start] = true; Q.Enqueue(Start);

	while (!Q.IsEmpty())
	{
		int32 Cur; Q.Dequeue(Cur);
		for (int32 N : UnderwaterCells[Cur].NeighborIndices)
		{
			if (!UnderwaterCells.IsValidIndex(N)||Assigned[N]||!UnderwaterCells[N].bUnderwater) continue;
			Assigned[N] = true; Q.Enqueue(N);

			const float Depth = Gen->CoastHeightThreshold - UnderwaterCells[N].SeabedZ;

			TSet<int32> VS; bool bHas = false;
			for (int32 NN : UnderwaterCells[N].NeighborIndices)
			{
				if (!UnderwaterCells.IsValidIndex(NN)||!Assigned[NN]) continue;
				bHas = true;
				const int32 Bi = UnderwaterBiomeRowNames.IndexOfByKey(UnderwaterCells[NN].BiomeRowName);
				if (!UnderwaterBiomeRows.IsValidIndex(Bi)) continue;
				const FVoxelUnderwaterBiomeRow& NB = UnderwaterBiomeRows[Bi];
				if (NB.CanBorderBiomes.IsEmpty())
					for (int32 B2=0; B2<UnderwaterBiomeRowNames.Num(); ++B2) VS.Add(B2);
				else
					for (int32 B2=0; B2<UnderwaterBiomeRowNames.Num(); ++B2)
						if (NB.CanBorderBiomes.Contains(UnderwaterBiomeRowNames[B2])) VS.Add(B2);
			}

			TArray<int32> VA = VS.Array();
			TArray<int32> DepthFiltered;
			for (int32 Bi : VA)
				if (Depth >= UnderwaterBiomeRows[Bi].PreferredMinDepth &&
				    Depth <= UnderwaterBiomeRows[Bi].PreferredMaxDepth)
					DepthFiltered.Add(Bi);
			if (!DepthFiltered.IsEmpty()) VA = DepthFiltered;

			if (!bHas || VA.IsEmpty())
			{
				TArray<int32> AllDepth;
				for (int32 Bi=0; Bi<UnderwaterBiomeRows.Num(); ++Bi)
					if (Depth >= UnderwaterBiomeRows[Bi].PreferredMinDepth &&
					    Depth <= UnderwaterBiomeRows[Bi].PreferredMaxDepth)
						AllDepth.Add(Bi);
				UnderwaterCells[N].BiomeRowName = UnderwaterBiomeRowNames[
					AllDepth.IsEmpty()
						? Rand.RandRange(0, UnderwaterBiomeRowNames.Num()-1)
						: AllDepth[Rand.RandRange(0, AllDepth.Num()-1)]];
			}
			else
			{
				UnderwaterCells[N].BiomeRowName = UnderwaterBiomeRowNames[VA[Rand.RandRange(0,VA.Num()-1)]];
			}
		}
	}
	
	for (int32 i=0; i<UnderwaterCells.Num(); ++i)
	{
		if (Assigned[i]||!UnderwaterCells[i].bUnderwater) continue;
		const float Depth = Gen->CoastHeightThreshold - UnderwaterCells[i].SeabedZ;
		TArray<int32> DepthValid;
		for (int32 Bi=0; Bi<UnderwaterBiomeRows.Num(); ++Bi)
			if (Depth >= UnderwaterBiomeRows[Bi].PreferredMinDepth &&
			    Depth <= UnderwaterBiomeRows[Bi].PreferredMaxDepth)
				DepthValid.Add(Bi);
		const int32 Pick = DepthValid.IsEmpty()
			? Rand.RandRange(0, UnderwaterBiomeRowNames.Num()-1)
			: DepthValid[Rand.RandRange(0, DepthValid.Num()-1)];
		UnderwaterCells[i].BiomeRowName = UnderwaterBiomeRowNames[Pick];
	}
}

void UVoxelUnderwaterNetworkComponent::AssignUnderwaterLocations()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	FRandomStream Rand(Gen->UnderwaterSeed + 2);
	TArray<FVector2D> AllPlaced;

	for (int32 Li=0; Li<UnderwaterLocationRows.Num(); ++Li)
	{
		const FVoxelUnderwaterLocationRow& Loc = UnderwaterLocationRows[Li];
		const FName& LocKey = UnderwaterLocationRowNames[Li];

		TArray<int32> Candidates;
		for (int32 Ci=0; Ci<UnderwaterCells.Num(); ++Ci)
		{
			const FVoxelUnderwaterCell& C = UnderwaterCells[Ci];
			if (!C.bUnderwater||C.LocationRowName!=NAME_None) continue;
			if (Loc.AllowedBiomes.Num()>0 && !Loc.AllowedBiomes.Contains(C.BiomeRowName)) continue;
			const float Depth = Gen->CoastHeightThreshold - C.SeabedZ;
			if (Depth < Loc.MinDepth || Depth > Loc.MaxDepth) continue;
			Candidates.Add(Ci);
		}
		for (int32 i=Candidates.Num()-1; i>0; --i) Candidates.Swap(i, Rand.RandRange(0,i));

		const float Spacings[3] = { Loc.MinSpacingFromOthers, Loc.MinSpacingFromOthers*0.5f, 0.f };
		bool bPlaced = false;

		for (int32 Pass=0; Pass<3&&!bPlaced; ++Pass)
		{
			const float SSq = Spacings[Pass]*Spacings[Pass];
			for (int32 Ci : Candidates)
			{
				const FVector2D P(UnderwaterCells[Ci].WorldPosition.X, UnderwaterCells[Ci].WorldPosition.Y);
				bool bClose = false;
				if (SSq>0.f)
					for (const FVector2D& E:AllPlaced)
						if (FVector2D::DistSquared(P,E)<SSq){bClose=true;break;}
				if (!bClose){UnderwaterCells[Ci].LocationRowName=LocKey;AllPlaced.Add(P);bPlaced=true;break;}
			}
		}
		if (bPlaced) continue;

		if (!Gen->bUnderwaterForcePlaceLocations)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("VoxelUnderwaterNetworkComponent: Could not place underwater '%s'."), *LocKey.ToString());
			continue;
		}

		TArray<int32> Fallback;
		for (int32 Ci=0; Ci<UnderwaterCells.Num(); ++Ci)
			if (UnderwaterCells[Ci].bUnderwater && UnderwaterCells[Ci].LocationRowName==NAME_None)
				Fallback.Add(Ci);
		if (Fallback.IsEmpty())
		{
			UE_LOG(LogTemp, Error,
				TEXT("VoxelUnderwaterNetworkComponent: Force-place underwater '%s' failed — no underwater cells!"),
				*LocKey.ToString());
			continue;
		}

		int32 BestCi=Fallback[0]; float BestMinDistSq=-1.f;
		for (int32 Ci : Fallback)
		{
			const FVector2D P(UnderwaterCells[Ci].WorldPosition.X, UnderwaterCells[Ci].WorldPosition.Y);
			float MinDistSq=FLT_MAX;
			if (AllPlaced.IsEmpty()){BestCi=Ci;break;}
			for (const FVector2D& E:AllPlaced) MinDistSq=FMath::Min(MinDistSq, FVector2D::DistSquared(P,E));
			if (MinDistSq>BestMinDistSq){BestMinDistSq=MinDistSq;BestCi=Ci;}
		}
		UnderwaterCells[BestCi].LocationRowName = LocKey;
		AllPlaced.Add(FVector2D(UnderwaterCells[BestCi].WorldPosition.X, UnderwaterCells[BestCi].WorldPosition.Y));
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelUnderwaterNetworkComponent: Force-placed underwater '%s' at cell %d."),
			*LocKey.ToString(), BestCi);
	}
}

void UVoxelUnderwaterNetworkComponent::RasterizeUnderwaterBiomeGrid()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	const int32 Res = Gen->GetCachedGridRes();
	CachedUnderwaterBiomeGrid.Init(NAME_None, Res*Res);
	if (UnderwaterCells.IsEmpty()) return;

	const FBox2D Bounds = Gen->GetNetworkBounds2D();
	const float SX = (Bounds.Max.X-Bounds.Min.X)/(Res-1);
	const float SY = (Bounds.Max.Y-Bounds.Min.Y)/(Res-1);
	const TArray<float>& HeightGrid = Gen->GetCachedHeightGrid();

	TArray<FVector2D> CP; CP.Reserve(UnderwaterCells.Num());
	for (const FVoxelUnderwaterCell& C : UnderwaterCells)
		CP.Add(FVector2D(C.WorldPosition.X, C.WorldPosition.Y));

	for (int32 Row=0; Row<Res; ++Row)
		for (int32 Col=0; Col<Res; ++Col)
		{
			const float H = HeightGrid[Row*Res+Col];
			if (H<=AVoxelSplineNetworkGenerator::GRID_MISS*0.5f || H>=Gen->CoastHeightThreshold || H<=Gen->UnderwaterFloorZ) continue;

			const FVector2D WXY(Bounds.Min.X+Col*SX, Bounds.Min.Y+Row*SY);
			float BD=FLT_MAX; int32 BC=-1;
			for (int32 Ci=0; Ci<CP.Num(); ++Ci)
			{
				if (!UnderwaterCells[Ci].bUnderwater) continue;
				const float D = FVector2D::DistSquared(WXY, CP[Ci]);
				if (D<BD){BD=D;BC=Ci;}
			}
			if (BC >= 0)
				CachedUnderwaterBiomeGrid[Row*Res+Col] = UnderwaterCells[BC].BiomeRowName;
		}
}

void UVoxelUnderwaterNetworkComponent::BuildUnderwaterCellSplines()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen || CachedUnderwaterBiomeGrid.IsEmpty()) return;
	const int32 Res = Gen->GetCachedGridRes();

	TSet<FName> PB;
	for (const FName& B : CachedUnderwaterBiomeGrid) if (B!=NAME_None) PB.Add(B);

	for (const FName& BN : PB)
	{
		const int32 Bi = UnderwaterBiomeRowNames.IndexOfByKey(BN);
		if (!UnderwaterBiomeRows.IsValidIndex(Bi)) continue;

		auto IF=[&](int32 R,int32 C)->bool{ return CachedUnderwaterBiomeGrid[R*Res+C]==BN; };
		auto VF=[&](int32 R,int32 C)->float{ return CachedUnderwaterBiomeGrid[R*Res+C]==BN?1.f:0.f; };
		const TArray<TArray<FVector2D>> Loops = FVoxelDelaunayVoronoiUtils::ExtractMarchingSquaresLoops(
			Res, Gen->GetNetworkBounds2D(), IF, VF, 0.5f, Gen->CoastMinLoopSegments);
		const FString SN = AVoxelSplineNetworkGenerator::SanitiseName(UnderwaterBiomeRows[Bi].BiomeName);
		int32 LC = 1;

		for (const TArray<FVector2D>& Loop : Loops)
		{
			USplineComponent* S = Gen->MakeSplineComponent(
				FString::Printf(TEXT("UW_Biome_%s_%02d"), *SN, LC++));
			for (const FVector2D& P : Loop)
			{
				FVector Hit;
				const float Z = Gen->TraceGroundAt(P.X, P.Y, Hit) ? Hit.Z : Gen->CoastHeightThreshold - 100.f;
				S->AddSplinePoint(FVector(P.X, P.Y, Z), ESplineCoordinateSpace::World, false);
			}
			S->SetClosedLoop(true,false); S->UpdateSpline();
			S->ComponentTags.Add(UnderwaterBiomeRows[Bi].PCGTag);
			UnderwaterBiomeSplines.Add(S);
		}
	}
	UE_LOG(LogTemp, Log,
		TEXT("VoxelUnderwaterNetworkComponent: %d underwater biome spline(s)."), UnderwaterBiomeSplines.Num());
}

void UVoxelUnderwaterNetworkComponent::BuildUnderwaterCentreSplines()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	const int32 Segs = 12;
	TMap<FString,int32> NC;

	for (int32 i=0; i<UnderwaterCells.Num(); ++i)
	{
		const FVoxelUnderwaterCell& Cell = UnderwaterCells[i];
		if (!Cell.bUnderwater) continue;

		const int32 Bi = UnderwaterBiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
		if (!UnderwaterBiomeRows.IsValidIndex(Bi)) continue;

		const bool  bIsLoc = Cell.LocationRowName != NAME_None;
		const int32 Li     = bIsLoc ? UnderwaterLocationRowNames.IndexOfByKey(Cell.LocationRowName) : INDEX_NONE;
		const float Radius = (bIsLoc&&UnderwaterLocationRows.IsValidIndex(Li))
		                   ? UnderwaterLocationRows[Li].CentreRadius : 8000.f;

		FString SplineName;
		if (bIsLoc&&UnderwaterLocationRows.IsValidIndex(Li))
			SplineName = FString::Printf(TEXT("UW_Centre_%s"), *AVoxelSplineNetworkGenerator::SanitiseName(UnderwaterLocationRows[Li].DisplayName));
		else
		{
			const FString S2 = AVoxelSplineNetworkGenerator::SanitiseName(UnderwaterBiomeRows[Bi].BiomeName);
			SplineName = FString::Printf(TEXT("UW_Centre_%s_%02d"), *S2, NC.FindOrAdd(S2)+++1);
		}

		USplineComponent* S = Gen->MakeSplineComponent(SplineName);
		for (int32 Seg=0; Seg<Segs; ++Seg)
		{
			const float A  = ((float)Seg/Segs)*2.f*PI;
			const float PX = Cell.WorldPosition.X + FMath::Cos(A)*Radius;
			const float PY = Cell.WorldPosition.Y + FMath::Sin(A)*Radius;
			FVector Hit;
			const float Z = Gen->TraceGroundAt(PX,PY,Hit) ? Hit.Z : Cell.SeabedZ;
			S->AddSplinePoint(FVector(PX,PY,Z), ESplineCoordinateSpace::World, false);
		}
		S->SetClosedLoop(true,false); S->UpdateSpline();
		S->ComponentTags.Add(UnderwaterBiomeRows[Bi].PCGTag);

		if (bIsLoc&&UnderwaterLocationRows.IsValidIndex(Li))
		{
			S->ComponentTags.Add(UnderwaterLocationRows[Li].PCGTag);
			S->ComponentTags.Add(UnderwaterLocationRows[Li].CategoryTag);
		}
		else S->ComponentTags.Add(TEXT("PCG_UW_CellCentre"));

		UnderwaterCentreSplines.Add(S);
	}
	UE_LOG(LogTemp, Log,
		TEXT("VoxelUnderwaterNetworkComponent: %d underwater centre spline(s)."), UnderwaterCentreSplines.Num());
}

void UVoxelUnderwaterNetworkComponent::ClearLabelComponents()
{
	for (UTextRenderComponent* C : LabelComponents) if (C) C->DestroyComponent();
	LabelComponents.Empty();
}

void UVoxelUnderwaterNetworkComponent::BuildLabelComponents()
{
	AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	for (int32 i=0; i<UnderwaterCells.Num(); ++i)
	{
		const FVoxelUnderwaterCell& Cell = UnderwaterCells[i];
		if (!Cell.bUnderwater) continue;

		FString Label; FColor Col = FColor(0, 140, 255);

		if (Cell.LocationRowName != NAME_None)
		{
			const int32 Li = UnderwaterLocationRowNames.IndexOfByKey(Cell.LocationRowName);
			if (UnderwaterLocationRows.IsValidIndex(Li))
			{
				const int32 Bi     = UnderwaterBiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
				const FString Biome = UnderwaterBiomeRows.IsValidIndex(Bi) ? UnderwaterBiomeRows[Bi].BiomeName : TEXT("?");
				const float Depth  = Gen->CoastHeightThreshold - Cell.SeabedZ;
				Label = FString::Printf(TEXT("[%s]\n%s\nDepth:%.0f"),
					*UnderwaterLocationRows[Li].DisplayName, *Biome, Depth);
				Col   = UnderwaterLocationRows[Li].DebugColor.ToFColor(true);
			}
		}
		else if (Gen->bShowUnderwaterBiomeLabelOnAllCells)
		{
			const int32 Bi = UnderwaterBiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
			if (UnderwaterBiomeRows.IsValidIndex(Bi))
			{
				Label = UnderwaterBiomeRows[Bi].BiomeName;
				Col   = UnderwaterBiomeRows[Bi].DebugColor.ToFColor(true);
			}
		}

		if (Label.IsEmpty()) continue;

		UTextRenderComponent* T = NewObject<UTextRenderComponent>(Gen, *FString::Printf(TEXT("UW_Label_%d"),i));
		T->SetupAttachment(Gen->GetRootComponent()); T->RegisterComponent();
		T->SetText(FText::FromString(Label));
		T->SetWorldLocation(Cell.WorldPosition + FVector(0.f, 0.f, Gen->DebugZLift+Gen->LabelZOffset));
		T->SetWorldSize(Gen->LabelWorldSize); T->SetTextRenderColor(Col);
		T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetWorldRotation(FRotator(90.f, 0.f, 180.f));
		LabelComponents.Add(T);
	}
}

void UVoxelUnderwaterNetworkComponent::DrawDebugEdges() const
{
	const AVoxelSplineNetworkGenerator* Gen = GetGenerator();
	if (!Gen) return;

	for (int32 i=0; i<UnderwaterCells.Num(); ++i)
	{
		if (!UnderwaterCells[i].bUnderwater) continue;
		const FVector F = UnderwaterCells[i].WorldPosition + FVector(0,0,Gen->DebugZLift);
		for (int32 N : UnderwaterCells[i].NeighborIndices)
		{
			if (N<=i||!UnderwaterCells.IsValidIndex(N)||!UnderwaterCells[N].bUnderwater) continue;
			DrawDebugLine(GetWorld(), F, UnderwaterCells[N].WorldPosition+FVector(0,0,Gen->DebugZLift),
				Gen->UnderwaterEdgeColor, false, Gen->DebugDuration, 0, Gen->UnderwaterEdgeThickness);
		}
	}
}