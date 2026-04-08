// VoxelSplineNetworkGenerator.cpp

#include "VoxelSplineNetworkGenerator.h"
#include "VoxelWorld.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "ProceduralMeshComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AVoxelSplineNetworkGenerator::AVoxelSplineNetworkGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FillMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FillMesh"));
	FillMeshComponent->SetupAttachment(Root);
	FillMeshComponent->bUseAsyncCooking = false;
	FillMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FillMeshComponent->SetCastShadow(false);

	Tags.Add(TEXT("VoxelSplineNetwork"));
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (bGenerateOnBeginPlay) GenerateNetwork();
}

void AVoxelSplineNetworkGenerator::Destroyed()
{
	// Cancel any pending delayed generation before tearing down
	if (GenerateDelayHandle.IsValid())
		GetWorldTimerManager().ClearTimer(GenerateDelayHandle);

	ClearNetwork();
	Super::Destroyed();
}

#if WITH_EDITOR
void AVoxelSplineNetworkGenerator::PostEditChangeProperty(FPropertyChangedEvent& E)
{
	Super::PostEditChangeProperty(E);
	if (!E.Property) return;
	const FName Prop = E.Property->GetFName();
	static const TSet<FName> DebugOnly = {
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowCellFill),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, CellFillBrightness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, CellFillMaterial),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowEdges),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, EdgeColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, EdgeThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowLabels),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowBiomeLabelOnAllCells),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, LabelZOffset),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, LabelWorldSize),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowRoads),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, RoadDebugColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, RoadDebugThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowCoast),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, CoastDebugColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, CoastDebugThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, DebugDuration),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, DebugZLift),
	};
	if (DebugOnly.Contains(Prop)) RefreshDebugDraw();
}
#endif

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::ClearNetwork()
{
	// Cancel any pending delayed generation so it doesn't fire into cleared state
	if (GenerateDelayHandle.IsValid())
		GetWorldTimerManager().ClearTimer(GenerateDelayHandle);

	Cells.Empty();
	CachedTriangles.Empty();
	CachedHeightGrid.Empty();
	CachedBiomeGrid.Empty();
	CachedGridRes = 0;
	BiomeRowNames.Empty(); BiomeRows.Empty();
	LocationRowNames.Empty(); LocationRows.Empty();

	for (USplineComponent* S : BiomeSplines)  if (S) S->DestroyComponent();
	for (USplineComponent* S : CentreSplines) if (S) S->DestroyComponent();
	for (USplineComponent* S : RoadSplines)   if (S) S->DestroyComponent();
	for (USplineComponent* S : CoastSplines)  if (S) S->DestroyComponent();
	BiomeSplines.Empty(); CentreSplines.Empty(); RoadSplines.Empty(); CoastSplines.Empty();

	if (FillMeshComponent) FillMeshComponent->ClearAllMeshSections();
	ClearLabelComponents();
	FlushPersistentDebugLines(GetWorld());
}

void AVoxelSplineNetworkGenerator::ClearNetworkRuntime() { ClearNetwork(); }

void AVoxelSplineNetworkGenerator::GenerateNetwork()
{
	// Cancel any pending delayed call — caller wants immediate generation
	if (GenerateDelayHandle.IsValid())
		GetWorldTimerManager().ClearTimer(GenerateDelayHandle);

	ClearNetwork();
	RunGenerationPipeline();
}

void AVoxelSplineNetworkGenerator::GenerateNetworkDelayed(float DelaySeconds)
{
	// Determine delay: negative sentinel means "use the property value"
	const float ActualDelay = (DelaySeconds >= 0.f) ? DelaySeconds : GenerationDelay;

	// Clear now so the old splines disappear immediately, not after the delay
	ClearNetwork();

	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: waiting %.2fs for voxel collision to settle..."), ActualDelay);

	// If delay is zero just run immediately (no timer overhead needed)
	if (ActualDelay <= 0.f)
	{
		RunGenerationPipeline();
		return;
	}

	GetWorldTimerManager().SetTimer(
		GenerateDelayHandle,
		this,
		&AVoxelSplineNetworkGenerator::RunGenerationPipeline,
		ActualDelay,
		false   // do not loop
	);
}

// ---------------------------------------------------------------------------
// Core pipeline (previously the body of GenerateNetwork after ClearNetwork)
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::RunGenerationPipeline()
{
	if (!CellFillMaterial)
	{
		CellFillMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(),
			nullptr, TEXT("/Voxel/Examples/Maps/Alfheim/Assets/Materials/MI_Debug_Cell.MI_Debug_Cell")));
		if (!CellFillMaterial)
			CellFillMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(),
				nullptr, TEXT("/Engine/EngineMaterials/VertexColorMaterial.VertexColorMaterial")));
	}

	LoadDataTables();
	if (BiomeRows.IsEmpty())
	{
		const FString Msg = TEXT("WARNING: VoxelSplineNetworkGenerator — BiomeDataTable is null or empty.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, Msg);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: %.0f x %.0f, %d biomes, %d locations."),
		NetworkWidth, NetworkWidth, BiomeRows.Num(), LocationRows.Num());

	GenerateCellPoints();
	BuildDelaunayGraph();
	ComputeVoronoiPolygons();
	AssignBiomes();
	TraceTerrainHeights();
	AssignLocations();

	SampleHeightGrid();
	RasterizeBiomeGrid();

	BuildCellSplines();
	BuildCentreSplines();
	BuildRoadSplines();
	if (bGenerateCoastSplines) BuildCoastlineSplines();
	RefreshDebugDraw();

	int32 LandCells = 0;
	for (const FVoxelBiomeCell& C : Cells) if (C.bOnTerrain) ++LandCells;
	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: Done. %d cells, %d on terrain, %d coast loops, %d biome splines."),
		Cells.Num(), LandCells, CoastSplines.Num(), BiomeSplines.Num());
}

void AVoxelSplineNetworkGenerator::RefreshDebugDraw()
{
	FlushPersistentDebugLines(GetWorld());
	RebuildFillMesh();
	if (bShowEdges)  DrawDebugEdges();
	DrawDebugLocationMarkers();
	ClearLabelComponents();
	if (bShowLabels) BuildLabelComponents();
	if (bShowRoads)  DrawDebugRoads();
	if (bShowCoast)  DrawDebugCoast();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

FString AVoxelSplineNetworkGenerator::SanitiseName(const FString& In)
{
	FString Out = In;
	for (const TCHAR Bad : { TEXT(' '), TEXT('/'), TEXT('\\'), TEXT('.') })
		Out.ReplaceCharInline(Bad, TEXT('_'), ESearchCase::CaseSensitive);
	return Out;
}

// ---------------------------------------------------------------------------
// DataTable loading
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::LoadDataTables()
{
	BiomeRowNames.Empty(); BiomeRows.Empty();
	LocationRowNames.Empty(); LocationRows.Empty();

	if (BiomeDataTable)
		for (const FName& Key : BiomeDataTable->GetRowNames())
			if (FVoxelBiomeRow* R = BiomeDataTable->FindRow<FVoxelBiomeRow>(Key, TEXT("")))
				{ BiomeRowNames.Add(Key); BiomeRows.Add(*R); }

	if (LocationDataTable)
		for (const FName& Key : LocationDataTable->GetRowNames())
			if (FVoxelLocationRow* R = LocationDataTable->FindRow<FVoxelLocationRow>(Key, TEXT("")))
				{ LocationRowNames.Add(Key); LocationRows.Add(*R); }
}

// ---------------------------------------------------------------------------
// Step 1 — Cell points
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::GenerateCellPoints()
{
	FRandomStream Rand(Seed);
	const int32 GridDim     = FMath::CeilToInt(FMath::Sqrt((float)CellCount));
	const float CellSpacing = NetworkWidth / FMath::Max(GridDim - 1, 1);
	const float HalfWidth   = NetworkWidth * 0.5f;
	const float JitterMax   = CellSpacing * JitterAmount * 0.5f;
	const FVector Origin    = GetActorLocation();

	Cells.Reserve(GridDim * GridDim);
	for (int32 Row = 0; Row < GridDim; ++Row)
		for (int32 Col = 0; Col < GridDim; ++Col)
		{
			FVoxelBiomeCell Cell;
			Cell.WorldPosition.X = Origin.X - HalfWidth + Col * CellSpacing + Rand.FRandRange(-JitterMax, JitterMax);
			Cell.WorldPosition.Y = Origin.Y - HalfWidth + Row * CellSpacing + Rand.FRandRange(-JitterMax, JitterMax);
			Cell.WorldPosition.Z = 0.f;
			Cells.Add(Cell);
		}
}

// ---------------------------------------------------------------------------
// Step 2 — Delaunay
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::BuildDelaunayGraph()
{
	if (Cells.Num() < 3) return;
	CachedTriangles = BowyerWatson();
	for (const FDelaunayTriangle& T : CachedTriangles)
	{
		auto Link = [&](int32 A, int32 B)
		{ if (Cells.IsValidIndex(A) && Cells.IsValidIndex(B)) Cells[A].NeighborIndices.AddUnique(B); };
		Link(T.A,T.B); Link(T.B,T.A); Link(T.B,T.C); Link(T.C,T.B); Link(T.A,T.C); Link(T.C,T.A);
	}
}

TArray<AVoxelSplineNetworkGenerator::FDelaunayTriangle>
AVoxelSplineNetworkGenerator::BowyerWatson() const
{
	TArray<FDelaunayTriangle> Tri;
	float MinX=FLT_MAX,MinY=FLT_MAX,MaxX=-FLT_MAX,MaxY=-FLT_MAX;
	for (const FVoxelBiomeCell& C : Cells)
	{ MinX=FMath::Min(MinX,C.WorldPosition.X); MinY=FMath::Min(MinY,C.WorldPosition.Y);
	  MaxX=FMath::Max(MaxX,C.WorldPosition.X); MaxY=FMath::Max(MaxY,C.WorldPosition.Y); }
	const float DMax=FMath::Max(MaxX-MinX,MaxY-MinY)*10.f;
	const float MidX=(MinX+MaxX)*.5f, MidY=(MinY+MaxY)*.5f;
	const int32 SA=Cells.Num(),SB=SA+1,SC=SA+2;
	TArray<FVector2D> Pts; Pts.Reserve(Cells.Num()+3);
	for (const FVoxelBiomeCell& C : Cells) Pts.Add(FVector2D(C.WorldPosition.X,C.WorldPosition.Y));
	Pts.Add(FVector2D(MidX-DMax,MidY-DMax));
	Pts.Add(FVector2D(MidX,     MidY+DMax));
	Pts.Add(FVector2D(MidX+DMax,MidY-DMax));
	Tri.Add({SA,SB,SC});
	for (int32 Pi=0;Pi<Cells.Num();++Pi)
	{
		const FVector2D& P=Pts[Pi];
		TArray<FDelaunayTriangle> Bad;
		for (const FDelaunayTriangle& T:Tri)
		{ FVector2D CC; float RR; GetCircumcircle(Pts[T.A],Pts[T.B],Pts[T.C],CC,RR);
		  if(FVector2D::DistSquared(CC,P)<=RR) Bad.Add(T); }
		TArray<FDelaunayEdge> Hole;
		for (const FDelaunayTriangle& B:Bad)
		{
			FDelaunayEdge E3[3]={{B.A,B.B},{B.B,B.C},{B.A,B.C}};
			for (const FDelaunayEdge& E:E3)
			{
				bool bS=false;
				for (const FDelaunayTriangle& O:Bad)
				{ if(&O==&B) continue; if((O.A==E.A||O.B==E.A||O.C==E.A)&&(O.A==E.B||O.B==E.B||O.C==E.B)){bS=true;break;} }
				if(!bS) Hole.Add(E);
			}
		}
		Tri.RemoveAll([&](const FDelaunayTriangle& T){ for(const FDelaunayTriangle& B:Bad) if(B.A==T.A&&B.B==T.B&&B.C==T.C) return true; return false; });
		for (const FDelaunayEdge& E:Hole) Tri.Add({E.A,E.B,Pi});
	}
	Tri.RemoveAll([&](const FDelaunayTriangle& T){return T.A>=SA||T.B>=SA||T.C>=SA;});
	return Tri;
}

void AVoxelSplineNetworkGenerator::GetCircumcircle(
	const FVector2D& A,const FVector2D& B,const FVector2D& C,FVector2D& OC,float& OR) const
{
	const float D=2.f*(A.X*(B.Y-C.Y)+B.X*(C.Y-A.Y)+C.X*(A.Y-B.Y));
	if(FMath::IsNearlyZero(D)){OC=(A+B+C)/3.f;OR=FLT_MAX;return;}
	const float A2=A.X*A.X+A.Y*A.Y,B2=B.X*B.X+B.Y*B.Y,C2=C.X*C.X+C.Y*C.Y;
	OC.X=(A2*(B.Y-C.Y)+B2*(C.Y-A.Y)+C2*(A.Y-B.Y))/D;
	OC.Y=(A2*(C.X-B.X)+B2*(A.X-C.X)+C2*(B.X-A.X))/D;
	OR=FVector2D::DistSquared(OC,A);
}

// ---------------------------------------------------------------------------
// Step 3 — Voronoi (kept for fill-mesh debug visualisation only)
// ---------------------------------------------------------------------------

FBox2D AVoxelSplineNetworkGenerator::GetNetworkBounds2D() const
{
	const FVector O=GetActorLocation(); const float H=NetworkWidth*.5f;
	return FBox2D(FVector2D(O.X-H,O.Y-H),FVector2D(O.X+H,O.Y+H));
}

TArray<FVector2D> AVoxelSplineNetworkGenerator::ComputeVoronoiPolygon2D(int32 Idx) const
{
	if (!Cells.IsValidIndex(Idx)) return {};
	const FVector2D Center(Cells[Idx].WorldPosition.X,Cells[Idx].WorldPosition.Y);
	const FBox2D B=GetNetworkBounds2D();
	TArray<TPair<float,FVector2D>> APs;
	for (const FDelaunayTriangle& T:CachedTriangles)
	{
		if(T.A!=Idx&&T.B!=Idx&&T.C!=Idx) continue;
		if(!Cells.IsValidIndex(T.A)||!Cells.IsValidIndex(T.B)||!Cells.IsValidIndex(T.C)) continue;
		FVector2D CC; float RR;
		GetCircumcircle(FVector2D(Cells[T.A].WorldPosition.X,Cells[T.A].WorldPosition.Y),
			FVector2D(Cells[T.B].WorldPosition.X,Cells[T.B].WorldPosition.Y),
			FVector2D(Cells[T.C].WorldPosition.X,Cells[T.C].WorldPosition.Y),CC,RR);
		CC.X=FMath::Clamp(CC.X,B.Min.X,B.Max.X); CC.Y=FMath::Clamp(CC.Y,B.Min.Y,B.Max.Y);
		APs.Add({FMath::Atan2(CC.Y-Center.Y,CC.X-Center.X),CC});
	}
	APs.Sort([](const TPair<float,FVector2D>& A,const TPair<float,FVector2D>& BB){return A.Key<BB.Key;});
	TArray<FVector2D> R; for(auto& AP:APs) R.Add(AP.Value); return R;
}

void AVoxelSplineNetworkGenerator::ComputeVoronoiPolygons()
{
	for(int32 i=0;i<Cells.Num();++i) Cells[i].VoronoiPolygon2D=ComputeVoronoiPolygon2D(i);
}

// ---------------------------------------------------------------------------
// Step 4 — Biome assignment
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::AssignBiomes()
{
	if(BiomeRows.IsEmpty()) return;
	FRandomStream Rand(Seed+1);
	TArray<bool> Assigned; Assigned.Init(false,Cells.Num());
	TQueue<int32> Q;
	const int32 Start=Rand.RandRange(0,Cells.Num()-1);
	Cells[Start].BiomeRowName=BiomeRowNames[Rand.RandRange(0,BiomeRowNames.Num()-1)];
	Assigned[Start]=true; Q.Enqueue(Start);
	while(!Q.IsEmpty())
	{
		int32 Cur; Q.Dequeue(Cur);
		for(int32 N:Cells[Cur].NeighborIndices)
		{
			if(Assigned[N]) continue; Assigned[N]=true; Q.Enqueue(N);
			TSet<int32> VS; bool bHas=false;
			for(int32 NN:Cells[N].NeighborIndices)
			{
				if(!Assigned[NN]) continue; bHas=true;
				const int32 NNBiomeIdx=BiomeRowNames.IndexOfByKey(Cells[NN].BiomeRowName);
				if(!BiomeRows.IsValidIndex(NNBiomeIdx)) continue;
				const FVoxelBiomeRow& NB=BiomeRows[NNBiomeIdx];
				if(NB.CanBorderBiomes.IsEmpty())
					for(int32 Bi=0;Bi<BiomeRowNames.Num();++Bi) VS.Add(Bi);
				else
					for(int32 Bi=0;Bi<BiomeRowNames.Num();++Bi)
						if(NB.CanBorderBiomes.Contains(BiomeRowNames[Bi])) VS.Add(Bi);
			}
			TArray<int32> VA=VS.Array();
			Cells[N].BiomeRowName=BiomeRowNames[(!bHas||VA.IsEmpty())?Rand.RandRange(0,BiomeRowNames.Num()-1):VA[Rand.RandRange(0,VA.Num()-1)]];
		}
	}
	for(int32 i=0;i<Cells.Num();++i)
		if(!Assigned[i]) Cells[i].BiomeRowName=BiomeRowNames[Rand.RandRange(0,BiomeRowNames.Num()-1)];
}

// ---------------------------------------------------------------------------
// Step 5 — Terrain heights
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::TraceTerrainHeights()
{
	for(FVoxelBiomeCell& C:Cells)
	{
		FVector Hit;
		if(TraceGroundAt(C.WorldPosition.X,C.WorldPosition.Y,Hit))
		{ C.WorldPosition.Z=Hit.Z; C.bOnTerrain=(Hit.Z>=MinSpawnHeight); }
	}
}

// ---------------------------------------------------------------------------
// Step 6 — Location assignment
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::AssignLocations()
{
	FRandomStream Rand(Seed+2);
	TArray<FVector2D> AllPlaced;
	for(int32 Li=0;Li<LocationRows.Num();++Li)
	{
		const FVoxelLocationRow& Loc=LocationRows[Li];
		const FName& LocKey=LocationRowNames[Li];
		TArray<int32> Candidates;
		for(int32 Ci=0;Ci<Cells.Num();++Ci)
		{
			const FVoxelBiomeCell& C=Cells[Ci];
			if(!C.bOnTerrain||C.LocationRowName!=NAME_None) continue;
			if(Loc.AllowedBiomes.Num()>0&&!Loc.AllowedBiomes.Contains(C.BiomeRowName)) continue;
			Candidates.Add(Ci);
		}
		for(int32 i=Candidates.Num()-1;i>0;--i) Candidates.Swap(i,Rand.RandRange(0,i));
		const float Spacings[3]={Loc.MinSpacingFromOthers,Loc.MinSpacingFromOthers*0.5f,0.f};
		bool bPlaced=false;
		for(int32 Pass=0;Pass<3&&!bPlaced;++Pass)
		{
			const float SSq=Spacings[Pass]*Spacings[Pass];
			for(int32 Ci:Candidates)
			{
				const FVector2D P(Cells[Ci].WorldPosition.X,Cells[Ci].WorldPosition.Y);
				bool bClose=false;
				if(SSq>0.f) for(const FVector2D& E:AllPlaced) if(FVector2D::DistSquared(P,E)<SSq){bClose=true;break;}
				if(!bClose){Cells[Ci].LocationRowName=LocKey;AllPlaced.Add(P);bPlaced=true;break;}
			}
		}
		if(!bPlaced) UE_LOG(LogTemp,Warning,TEXT("VoxelSplineNetworkGenerator: Could not place '%s'."),*LocKey.ToString());
	}
}

// ---------------------------------------------------------------------------
// Step 6b — Sample height grid
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::SampleHeightGrid()
{
	const int32 Res = FMath::Max(CoastSampleResolution, 2);
	CachedGridRes = Res;
	CachedHeightGrid.SetNum(Res * Res);
	const FBox2D Bounds = GetNetworkBounds2D();
	const float StepX = (Bounds.Max.X - Bounds.Min.X) / (Res - 1);
	const float StepY = (Bounds.Max.Y - Bounds.Min.Y) / (Res - 1);

	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: Sampling %d x %d height grid..."), Res, Res);

	for (int32 Row = 0; Row < Res; ++Row)
	{
		const float WY = Bounds.Min.Y + Row * StepY;
		for (int32 Col = 0; Col < Res; ++Col)
		{
			FVector Hit;
			CachedHeightGrid[Row * Res + Col] = TraceGroundAt(Bounds.Min.X + Col * StepX, WY, Hit) ? Hit.Z : GRID_MISS;
		}
	}
}

// ---------------------------------------------------------------------------
// Step 6c — Rasterise biome assignments onto the height grid
//
// Uses CoastHeightThreshold (not MinSpawnHeight) as the land gate so biome
// and coast boundaries share a single consistent definition of "land".
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::RasterizeBiomeGrid()
{
	const int32 Res = CachedGridRes;
	CachedBiomeGrid.Init(NAME_None, Res * Res);
	if (Cells.IsEmpty()) return;

	const FBox2D Bounds = GetNetworkBounds2D();
	const float StepX = (Bounds.Max.X - Bounds.Min.X) / (Res - 1);
	const float StepY = (Bounds.Max.Y - Bounds.Min.Y) / (Res - 1);

	TArray<FVector2D> CellPos2D;
	CellPos2D.Reserve(Cells.Num());
	for (const FVoxelBiomeCell& C : Cells)
		CellPos2D.Add(FVector2D(C.WorldPosition.X, C.WorldPosition.Y));

	for (int32 Row = 0; Row < Res; ++Row)
	{
		for (int32 Col = 0; Col < Res; ++Col)
		{
			const float H = CachedHeightGrid[Row * Res + Col];
			if (H <= GRID_MISS * 0.5f || H < CoastHeightThreshold) continue;

			const FVector2D WorldXY(Bounds.Min.X + Col * StepX, Bounds.Min.Y + Row * StepY);

			float BestDistSq = FLT_MAX;
			int32 BestCell   = 0;
			for (int32 Ci = 0; Ci < CellPos2D.Num(); ++Ci)
			{
				const float DSq = FVector2D::DistSquared(WorldXY, CellPos2D[Ci]);
				if (DSq < BestDistSq) { BestDistSq = DSq; BestCell = Ci; }
			}
			CachedBiomeGrid[Row * Res + Col] = Cells[BestCell].BiomeRowName;
		}
	}
}

// ---------------------------------------------------------------------------
// Core helper — Marching Squares loop extractor
// ---------------------------------------------------------------------------

TArray<TArray<FVector2D>> AVoxelSplineNetworkGenerator::ExtractMarchingSquaresLoops(
	TFunctionRef<bool(int32 Row, int32 Col)> InsideFn,
	TFunctionRef<float(int32 Row, int32 Col)> ValueFn,
	float Threshold) const
{
	const int32 Res = CachedGridRes;
	const FBox2D Bounds = GetNetworkBounds2D();
	const float StepX = (Bounds.Max.X - Bounds.Min.X) / (Res - 1);
	const float StepY = (Bounds.Max.Y - Bounds.Min.Y) / (Res - 1);

	auto GridPos = [&](int32 Row, int32 Col) -> FVector2D
	{
		return FVector2D(Bounds.Min.X + Col * StepX, Bounds.Min.Y + Row * StepY);
	};

	auto Interp = [&](int32 R0, int32 C0, int32 R1, int32 C1) -> FVector2D
	{
		const float V0 = ValueFn(R0, C0);
		const float V1 = ValueFn(R1, C1);
		float T = 0.5f;
		const float Denom = V0 - V1;
		if (!FMath::IsNearlyZero(Denom))
			T = FMath::Clamp((V0 - Threshold) / Denom, 0.f, 1.f);
		return FMath::Lerp(GridPos(R0, C0), GridPos(R1, C1), T);
	};

	struct FSeg { FVector2D P0, P1; };
	TArray<FSeg> Segs;
	Segs.Reserve((Res - 1) * (Res - 1));

	for (int32 Row = 0; Row < Res - 1; ++Row)
	{
		for (int32 Col = 0; Col < Res - 1; ++Col)
		{
			const bool TL = InsideFn(Row,   Col);
			const bool TR = InsideFn(Row,   Col+1);
			const bool BR = InsideFn(Row+1, Col+1);
			const bool BL = InsideFn(Row+1, Col);
			const int32 Case = (TL?1:0)|(TR?2:0)|(BR?4:0)|(BL?8:0);
			if (Case == 0 || Case == 15) continue;

			auto Top    = [&]{ return Interp(Row,   Col,   Row,   Col+1); };
			auto Right  = [&]{ return Interp(Row,   Col+1, Row+1, Col+1); };
			auto Bottom = [&]{ return Interp(Row+1, Col,   Row+1, Col+1); };
			auto Left   = [&]{ return Interp(Row,   Col,   Row+1, Col); };

			switch (Case)
			{
				case  1: Segs.Add({Top(),    Left()  }); break;
				case  2: Segs.Add({Right(),  Top()   }); break;
				case  4: Segs.Add({Bottom(), Right() }); break;
				case  8: Segs.Add({Left(),   Bottom()}); break;
				case 14: Segs.Add({Top(),    Left()  }); break;
				case 13: Segs.Add({Right(),  Top()   }); break;
				case 11: Segs.Add({Bottom(), Right() }); break;
				case  7: Segs.Add({Left(),   Bottom()}); break;
				case  3: Segs.Add({Right(),  Left()  }); break;
				case  6: Segs.Add({Bottom(), Top()   }); break;
				case 12: Segs.Add({Right(),  Left()  }); break;
				case  9: Segs.Add({Bottom(), Top()   }); break;
				case  5: Segs.Add({Top(),    Right() }); Segs.Add({Bottom(), Left()  }); break;
				case 10: Segs.Add({Right(),  Bottom()}); Segs.Add({Left(),   Top()   }); break;
				default: break;
			}
		}
	}

	if (Segs.IsEmpty()) return {};

	auto PosKey = [](const FVector2D& P) -> uint64
	{
		return ((uint64)(uint32)FMath::RoundToInt(P.X) << 32)
		      | (uint64)(uint32)FMath::RoundToInt(P.Y);
	};

	TMap<uint64, TArray<TPair<int32,int32>>> PosMap;
	PosMap.Reserve(Segs.Num() * 2);
	for (int32 Si = 0; Si < Segs.Num(); ++Si)
	{
		PosMap.FindOrAdd(PosKey(Segs[Si].P0)).Add({Si,0});
		PosMap.FindOrAdd(PosKey(Segs[Si].P1)).Add({Si,1});
	}

	TArray<bool> Used; Used.Init(false, Segs.Num());
	TArray<TArray<FVector2D>> Loops;

	for (int32 StartSeg = 0; StartSeg < Segs.Num(); ++StartSeg)
	{
		if (Used[StartSeg]) continue;

		TArray<FVector2D> Loop;
		Loop.Add(Segs[StartSeg].P0);
		Used[StartSeg] = true;
		FVector2D Cur = Segs[StartSeg].P1;
		const uint64 StartKey = PosKey(Segs[StartSeg].P0);
		bool bClosed = false;

		for (int32 Iter = 0; Iter < Segs.Num(); ++Iter)
		{
			const uint64 CurKey = PosKey(Cur);
			if (CurKey == StartKey && Loop.Num() > 2) { bClosed = true; break; }
			const auto* Neighbors = PosMap.Find(CurKey);
			if (!Neighbors) break;
			bool bFound = false;
			for (const TPair<int32,int32>& NP : *Neighbors)
			{
				if (Used[NP.Key]) continue;
				Used[NP.Key] = true;
				Loop.Add(Cur);
				Cur = (NP.Value == 0) ? Segs[NP.Key].P1 : Segs[NP.Key].P0;
				bFound = true;
				break;
			}
			if (!bFound) break;
		}

		if (bClosed && Loop.Num() >= CoastMinLoopSegments)
			Loops.Add(MoveTemp(Loop));
	}

	return Loops;
}

// ---------------------------------------------------------------------------
// Step 7 — Biome outline splines (Marching Squares on CachedBiomeGrid)
// Boundary points over ocean use bilinear CachedHeightGrid fallback (Bug 1 fix)
// ---------------------------------------------------------------------------

USplineComponent* AVoxelSplineNetworkGenerator::MakeSplineComponent(const FString& Name)
{
	USplineComponent* S = NewObject<USplineComponent>(this, *Name);
	S->SetupAttachment(GetRootComponent());
	S->RegisterComponent();
	AddInstanceComponent(S);
	S->ClearSplinePoints(false);
	return S;
}

void AVoxelSplineNetworkGenerator::BuildCellSplines()
{
	if (CachedBiomeGrid.IsEmpty()) return;

	const int32 Res = CachedGridRes;
	const FBox2D Bounds = GetNetworkBounds2D();
	const float StepX = (Bounds.Max.X - Bounds.Min.X) / (Res - 1);
	const float StepY = (Bounds.Max.Y - Bounds.Min.Y) / (Res - 1);

	auto SampleCachedHeight = [&](float WX, float WY) -> float
	{
		const float FCol = (WX - Bounds.Min.X) / StepX;
		const float FRow = (WY - Bounds.Min.Y) / StepY;
		const int32 C0 = FMath::Clamp(FMath::FloorToInt(FCol), 0, Res - 1);
		const int32 R0 = FMath::Clamp(FMath::FloorToInt(FRow), 0, Res - 1);
		const int32 C1 = FMath::Min(C0 + 1, Res - 1);
		const int32 R1 = FMath::Min(R0 + 1, Res - 1);
		const float TX = FMath::Clamp(FCol - C0, 0.f, 1.f);
		const float TY = FMath::Clamp(FRow - R0, 0.f, 1.f);

		auto SafeH = [&](int32 R, int32 C) -> float
		{
			const float H = CachedHeightGrid[R * Res + C];
			return (H > GRID_MISS * 0.5f) ? H : GRID_MISS;
		};

		const float H00 = SafeH(R0, C0), H10 = SafeH(R0, C1);
		const float H01 = SafeH(R1, C0), H11 = SafeH(R1, C1);

		float Valid = GRID_MISS;
		for (float H : {H00, H10, H01, H11})
			if (H > GRID_MISS * 0.5f) { Valid = H; break; }

		const float Top    = (H00 > GRID_MISS * 0.5f && H10 > GRID_MISS * 0.5f) ? FMath::Lerp(H00, H10, TX) : Valid;
		const float Bottom = (H01 > GRID_MISS * 0.5f && H11 > GRID_MISS * 0.5f) ? FMath::Lerp(H01, H11, TX) : Valid;
		return (Top > GRID_MISS * 0.5f && Bottom > GRID_MISS * 0.5f) ? FMath::Lerp(Top, Bottom, TY) : Valid;
	};

	TSet<FName> PresentBiomes;
	for (const FName& B : CachedBiomeGrid)
		if (B != NAME_None) PresentBiomes.Add(B);

	for (const FName& BiomeName : PresentBiomes)
	{
		const int32 BiomeIdx = BiomeRowNames.IndexOfByKey(BiomeName);
		if (!BiomeRows.IsValidIndex(BiomeIdx)) continue;

		auto InsideFn = [&](int32 Row, int32 Col) -> bool
		{
			return CachedBiomeGrid[Row * Res + Col] == BiomeName;
		};

		auto ValueFn = [&](int32 Row, int32 Col) -> float
		{
			return CachedBiomeGrid[Row * Res + Col] == BiomeName ? 1.f : 0.f;
		};

		const TArray<TArray<FVector2D>> Loops = ExtractMarchingSquaresLoops(InsideFn, ValueFn, 0.5f);

		const FString SafeName = SanitiseName(BiomeRows[BiomeIdx].BiomeName);
		int32 LoopCount = 1;

		for (const TArray<FVector2D>& Loop : Loops)
		{
			const FString SplineName = FString::Printf(TEXT("Biome_%s_%02d"), *SafeName, LoopCount++);
			USplineComponent* S = MakeSplineComponent(SplineName);

			for (const FVector2D& P : Loop)
			{
				FVector Hit;
				float Z;
				if (TraceGroundAt(P.X, P.Y, Hit))
				{
					Z = Hit.Z + RoadZOffset;
				}
				else
				{
					const float CachedH = SampleCachedHeight(P.X, P.Y);
					Z = (CachedH > GRID_MISS * 0.5f) ? CachedH + RoadZOffset : RoadZOffset;
				}
				S->AddSplinePoint(FVector(P.X, P.Y, Z), ESplineCoordinateSpace::World, false);
			}

			S->SetClosedLoop(true, false);
			S->UpdateSpline();
			S->ComponentTags.Add(BiomeRows[BiomeIdx].PCGTag);
			BiomeSplines.Add(S);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: %d biome splines built."), BiomeSplines.Num());
}

// ---------------------------------------------------------------------------
// Step 7b — Centre splines
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::BuildCentreSplines()
{
	const int32 Segs = 12;
	TMap<FString, int32> NameCount;

	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		const FVoxelBiomeCell& Cell = Cells[i];
		if (!Cell.bOnTerrain) continue;
		const int32 BiomeIdx = BiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
		if (!BiomeRows.IsValidIndex(BiomeIdx)) continue;

		const bool bIsLoc = Cell.LocationRowName != NAME_None;
		const int32 LocIdx = bIsLoc ? LocationRowNames.IndexOfByKey(Cell.LocationRowName) : INDEX_NONE;
		const float Radius = bIsLoc && LocationRows.IsValidIndex(LocIdx) ? LocationRows[LocIdx].CentreRadius : 10000.f;

		FString SplineName;
		if (bIsLoc && LocationRows.IsValidIndex(LocIdx))
			SplineName = FString::Printf(TEXT("Centre_%s"), *SanitiseName(LocationRows[LocIdx].DisplayName));
		else
		{
			const FString Safe = SanitiseName(BiomeRows[BiomeIdx].BiomeName);
			SplineName = FString::Printf(TEXT("Centre_%s_%02d"), *Safe, NameCount.FindOrAdd(Safe)++ + 1);
		}

		USplineComponent* S = MakeSplineComponent(SplineName);
		for (int32 Seg = 0; Seg < Segs; ++Seg)
		{
			const float A = ((float)Seg / Segs) * 2.f * PI;
			const float PX = Cell.WorldPosition.X + FMath::Cos(A) * Radius;
			const float PY = Cell.WorldPosition.Y + FMath::Sin(A) * Radius;
			FVector Hit;
			const float Z = TraceGroundAt(PX, PY, Hit) ? Hit.Z + RoadZOffset : Cell.WorldPosition.Z + RoadZOffset;
			S->AddSplinePoint(FVector(PX, PY, Z), ESplineCoordinateSpace::World, false);
		}
		S->SetClosedLoop(true, false); S->UpdateSpline();
		S->ComponentTags.Add(BiomeRows[BiomeIdx].PCGTag);
		if (bIsLoc && LocationRows.IsValidIndex(LocIdx))
		{ S->ComponentTags.Add(LocationRows[LocIdx].PCGTag); S->ComponentTags.Add(LocationRows[LocIdx].CategoryTag); }
		else
			S->ComponentTags.Add(TEXT("PCG_CellCentre"));
		CentreSplines.Add(S);
	}
}

// ---------------------------------------------------------------------------
// Step 8 — Road splines
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::BuildRoadSplines()
{
	TArray<int32> RoadNodes;
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		if (Cells[i].LocationRowName == NAME_None) continue;
		const int32 Li = LocationRowNames.IndexOfByKey(Cells[i].LocationRowName);
		if (LocationRows.IsValidIndex(Li) && LocationRows[Li].bConnectsToRoadNetwork) RoadNodes.Add(i);
	}

	TSet<uint64> Done;
	for (int32 i = 0; i < RoadNodes.Num(); ++i)
		for (int32 j = i + 1; j < RoadNodes.Num(); ++j)
		{
			const int32 A = RoadNodes[i], B = RoadNodes[j];
			const uint64 K = ((uint64)FMath::Min(A,B) << 32) | (uint64)FMath::Max(A,B);
			if (Done.Contains(K)) continue;
			TArray<int32> Path = BFSPath(A, B);
			if (Path.Num() > 1 && Path.Num() <= MaxRoadPathHops + 1)
			{
				const int32 LiA = LocationRowNames.IndexOfByKey(Cells[A].LocationRowName);
				const int32 LiB = LocationRowNames.IndexOfByKey(Cells[B].LocationRowName);
				const FString NA = LocationRows.IsValidIndex(LiA) ? SanitiseName(LocationRows[LiA].DisplayName) : FString::Printf(TEXT("Cell%d"), A);
				const FString NB = LocationRows.IsValidIndex(LiB) ? SanitiseName(LocationRows[LiB].DisplayName) : FString::Printf(TEXT("Cell%d"), B);
				CreateRoadSpline(Path, NA, NB); Done.Add(K);
			}
		}
}

TArray<int32> AVoxelSplineNetworkGenerator::BFSPath(int32 Start, int32 End) const
{
	if (Start == End) return {Start};
	TMap<int32,int32> CF; TQueue<int32> Q; CF.Add(Start,-1); Q.Enqueue(Start);
	while (!Q.IsEmpty())
	{
		int32 Cur; Q.Dequeue(Cur);
		if (Cur == End) { TArray<int32> P; for (int32 N=End; N!=-1; N=CF[N]) P.Insert(N,0); return P; }
		for (int32 N : Cells[Cur].NeighborIndices)
		{
			if (CF.Contains(N) || !Cells.IsValidIndex(N) || !Cells[N].bOnTerrain) continue;
			if (Cells[N].WorldPosition.Z < MinRoadHeight) continue;
			CF.Add(N,Cur); Q.Enqueue(N);
		}
	}
	return {};
}

void AVoxelSplineNetworkGenerator::CreateRoadSpline(const TArray<int32>& Path, const FString& NameA, const FString& NameB)
{
	if (Path.Num() < 2) return;
	USplineComponent* S = MakeSplineComponent(FString::Printf(TEXT("Road_%s_to_%s_%02d"), *NameA, *NameB, RoadSplines.Num()+1));
	for (int32 Pi = 0; Pi < Path.Num(); ++Pi)
	{
		const int32 Ci = Path[Pi]; if (!Cells.IsValidIndex(Ci)) continue;
		FVector Hit;
		const float Z = TraceGroundAt(Cells[Ci].WorldPosition.X, Cells[Ci].WorldPosition.Y, Hit)
			? Hit.Z + RoadZOffset : Cells[Ci].WorldPosition.Z + RoadZOffset;
		S->AddSplinePoint(FVector(Cells[Ci].WorldPosition.X, Cells[Ci].WorldPosition.Y, Z), ESplineCoordinateSpace::World, false);

		if (Pi + 1 < Path.Num() && RoadIntermediateSamples > 0)
		{
			const int32 Ni = Path[Pi+1]; if (!Cells.IsValidIndex(Ni)) continue;
			for (int32 Si = 1; Si < RoadIntermediateSamples; ++Si)
			{
				const float t = (float)Si / RoadIntermediateSamples;
				const FVector L = FMath::Lerp(Cells[Ci].WorldPosition, Cells[Ni].WorldPosition, t);
				FVector H2;
				const float Z2 = TraceGroundAt(L.X, L.Y, H2) ? H2.Z + RoadZOffset : L.Z + RoadZOffset;
				S->AddSplinePoint(FVector(L.X, L.Y, Z2), ESplineCoordinateSpace::World, false);
			}
		}
	}
	S->UpdateSpline();
	S->ComponentTags.Add(TEXT("PCG_Road"));
	RoadSplines.Add(S);
}

// ---------------------------------------------------------------------------
// Step 9 — Coastline splines (Marching Squares on CachedHeightGrid)
// Per-point trace; threshold+offset used only as fallback (Bug 3 fix)
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::BuildCoastlineSplines()
{
	if (CachedHeightGrid.IsEmpty()) return;

	const int32 Res = CachedGridRes;
	const float Threshold = CoastHeightThreshold;

	auto InsideFn = [&](int32 Row, int32 Col) -> bool
	{
		const float H = CachedHeightGrid[Row * Res + Col];
		return H > GRID_MISS * 0.5f && H >= Threshold;
	};

	auto ValueFn = [&](int32 Row, int32 Col) -> float
	{
		const float H = CachedHeightGrid[Row * Res + Col];
		return (H > GRID_MISS * 0.5f) ? H : GRID_MISS;
	};

	const TArray<TArray<FVector2D>> Loops = ExtractMarchingSquaresLoops(InsideFn, ValueFn, Threshold);

	for (const TArray<FVector2D>& Loop : Loops)
	{
		const FString SplineName = FString::Printf(TEXT("Coast_%02d"), CoastSplines.Num() + 1);
		USplineComponent* S = MakeSplineComponent(SplineName);

		for (const FVector2D& P : Loop)
		{
			FVector Hit;
			const float Z = TraceGroundAt(P.X, P.Y, Hit)
				? Hit.Z + CoastZOffset
				: Threshold + CoastZOffset;
			S->AddSplinePoint(FVector(P.X, P.Y, Z), ESplineCoordinateSpace::World, false);
		}

		S->SetClosedLoop(true, false);
		S->UpdateSpline();
		S->ComponentTags.Add(TEXT("PCG_Coast"));
		CoastSplines.Add(S);
	}

	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: %d coastline loop(s) generated."), CoastSplines.Num());
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

void AVoxelSplineNetworkGenerator::RebuildFillMesh()
{
	if (!FillMeshComponent) return;
	FillMeshComponent->ClearAllMeshSections();
	if (!bShowCellFill) return;

	for (int32 Ci = 0; Ci < Cells.Num(); ++Ci)
	{
		const FVoxelBiomeCell& Cell = Cells[Ci];
		if (!Cell.bOnTerrain || Cell.VoronoiPolygon2D.Num() < 3) continue;
		const FColor Col = GetBiomeColor(Cell.BiomeRowName, CellFillBrightness);
		FVector CH;
		const float CZ = TraceGroundAt(Cell.WorldPosition.X, Cell.WorldPosition.Y, CH) ? CH.Z + DebugZLift : Cell.WorldPosition.Z + DebugZLift;
		TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Norms; TArray<FVector2D> UVs; TArray<FColor> VCs;
		Verts.Add(FVector(Cell.WorldPosition.X, Cell.WorldPosition.Y, CZ));
		Norms.Add(FVector::UpVector); UVs.Add(FVector2D(.5f,.5f)); VCs.Add(Col);
		const int32 N = Cell.VoronoiPolygon2D.Num();
		for (int32 Vi = 0; Vi < N; ++Vi)
		{
			const FVector2D& V = Cell.VoronoiPolygon2D[Vi];
			FVector Hit; const float Z = TraceGroundAt(V.X, V.Y, Hit) ? Hit.Z + DebugZLift : CZ;
			Verts.Add(FVector(V.X, V.Y, Z)); Norms.Add(FVector::UpVector);
			UVs.Add(FVector2D((float)Vi/N, 1.f)); VCs.Add(Col);
		}
		for (int32 Vi = 0; Vi < N; ++Vi)
		{ const int32 A=1+Vi, B=1+(Vi+1)%N; Tris.Add(0); Tris.Add(A); Tris.Add(B); Tris.Add(0); Tris.Add(B); Tris.Add(A); }
		TArray<FProcMeshTangent> Tans;
		FillMeshComponent->CreateMeshSection(Ci, Verts, Tris, Norms, UVs, VCs, Tans, false);
		if (CellFillMaterial) FillMeshComponent->SetMaterial(Ci, CellFillMaterial);
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugEdges() const
{
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		if (!Cells[i].bOnTerrain) continue;
		const FVector F = Cells[i].WorldPosition + FVector(0,0,DebugZLift);
		for (int32 N : Cells[i].NeighborIndices)
		{
			if (N <= i || !Cells.IsValidIndex(N) || !Cells[N].bOnTerrain) continue;
			DrawDebugLine(GetWorld(), F, Cells[N].WorldPosition + FVector(0,0,DebugZLift), EdgeColor, false, DebugDuration, 0, EdgeThickness);
		}
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugLocationMarkers() const
{
	for (const FVoxelBiomeCell& Cell : Cells)
	{
		if (!Cell.bOnTerrain || Cell.LocationRowName == NAME_None) continue;
		const int32 Li = LocationRowNames.IndexOfByKey(Cell.LocationRowName);
		if (!LocationRows.IsValidIndex(Li)) continue;
		DrawDebugSphere(GetWorld(), Cell.WorldPosition + FVector(0,0,DebugZLift),
			LocationRows[Li].CentreRadius, 12, LocationRows[Li].DebugColor.ToFColor(true), false, DebugDuration);
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugRoads() const
{
	for (USplineComponent* Spline : RoadSplines)
	{
		if (!Spline) continue;
		const int32 Steps = FMath::Max(Spline->GetNumberOfSplinePoints() * 8, 20);
		const float Len = Spline->GetSplineLength();
		for (int32 s = 0; s < Steps; ++s)
		{
			const FVector P0 = Spline->GetLocationAtDistanceAlongSpline(((float)s/Steps)*Len, ESplineCoordinateSpace::World);
			const FVector P1 = Spline->GetLocationAtDistanceAlongSpline(((float)(s+1)/Steps)*Len, ESplineCoordinateSpace::World);
			DrawDebugLine(GetWorld(), P0, P1, RoadDebugColor, false, DebugDuration, 0, RoadDebugThickness);
		}
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugCoast() const
{
	for (USplineComponent* Spline : CoastSplines)
	{
		if (!Spline) continue;
		const int32 Steps = FMath::Max(Spline->GetNumberOfSplinePoints() * 4, 20);
		const float Len = Spline->GetSplineLength();
		for (int32 s = 0; s < Steps; ++s)
		{
			const FVector P0 = Spline->GetLocationAtDistanceAlongSpline(((float)s/Steps)*Len, ESplineCoordinateSpace::World);
			const FVector P1 = Spline->GetLocationAtDistanceAlongSpline(((float)(s+1)/Steps)*Len, ESplineCoordinateSpace::World);
			DrawDebugLine(GetWorld(), P0, P1, CoastDebugColor, false, DebugDuration, 0, CoastDebugThickness);
		}
	}
}

void AVoxelSplineNetworkGenerator::ClearLabelComponents()
{
	for (UTextRenderComponent* C : LabelComponents) if (C) C->DestroyComponent();
	LabelComponents.Empty();
}

void AVoxelSplineNetworkGenerator::BuildLabelComponents()
{
	int32 Idx = 0;
	for (const FVoxelBiomeCell& Cell : Cells)
	{
		if (!Cell.bOnTerrain) continue;
		FString Label; FColor Col = FColor::White;
		if (Cell.LocationRowName != NAME_None)
		{
			const int32 Li = LocationRowNames.IndexOfByKey(Cell.LocationRowName);
			if (LocationRows.IsValidIndex(Li))
			{
				const int32 Bi = BiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
				Label = FString::Printf(TEXT("[%s]\n%s"), *LocationRows[Li].DisplayName,
					BiomeRows.IsValidIndex(Bi) ? *BiomeRows[Bi].BiomeName : TEXT("?"));
				Col = LocationRows[Li].DebugColor.ToFColor(true);
			}
		}
		else if (bShowBiomeLabelOnAllCells)
		{
			const int32 Bi = BiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
			if (BiomeRows.IsValidIndex(Bi)) { Label = BiomeRows[Bi].BiomeName; Col = GetBiomeColor(Cell.BiomeRowName); }
		}
		if (Label.IsEmpty()) continue;
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(this, *FString::Printf(TEXT("Label_%d"), Idx++));
		T->SetupAttachment(GetRootComponent()); T->RegisterComponent();
		T->SetText(FText::FromString(Label));
		T->SetWorldLocation(Cell.WorldPosition + FVector(0.f, 0.f, DebugZLift + LabelZOffset));
		T->SetWorldSize(LabelWorldSize); T->SetTextRenderColor(Col);
		T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetWorldRotation(FRotator(90.f, 0.f, 180.f));
		LabelComponents.Add(T);
	}
}

// ---------------------------------------------------------------------------
// Terrain
// ---------------------------------------------------------------------------

AVoxelWorld* AVoxelSplineNetworkGenerator::ResolveVoxelWorld() const
{
	if (VoxelWorld) return VoxelWorld;
	return Cast<AVoxelWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass()));
}

bool AVoxelSplineNetworkGenerator::TraceGroundAt(float X, float Y, FVector& OutHit) const
{
	FHitResult Hit; FCollisionQueryParams P; P.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(X,Y,TraceStartZ), FVector(X,Y,TraceEndZ), ECC_WorldStatic, P))
	{ OutHit = Hit.ImpactPoint; return true; }
	return false;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

FColor AVoxelSplineNetworkGenerator::GetBiomeColor(FName RowName, float Br) const
{
	const int32 Idx = BiomeRowNames.IndexOfByKey(RowName);
	if (!BiomeRows.IsValidIndex(Idx)) return FColor::White;
	return (BiomeRows[Idx].DebugColor * Br).ToFColor(false);
}

FColor AVoxelSplineNetworkGenerator::GetLocationColor(FName RowName) const
{
	const int32 Idx = LocationRowNames.IndexOfByKey(RowName);
	if (!LocationRows.IsValidIndex(Idx)) return FColor::Yellow;
	return LocationRows[Idx].DebugColor.ToFColor(true);
}