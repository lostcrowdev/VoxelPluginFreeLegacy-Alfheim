// VoxelSplineNetworkGenerator.cpp

#include "Alfheim/VoxelSplineNetworkGenerator.h"

#include "VoxelWorld.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "ProceduralMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/UnrealType.h"

using FDelaunayTriangle = FVoxelDelaunayVoronoiUtils::FDelaunayTriangle;

AVoxelSplineNetworkGenerator::AVoxelSplineNetworkGenerator()
{
	PrimaryActorTick.bCanEverTick          = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FillMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FillMesh"));
	FillMeshComponent->SetupAttachment(Root);
	FillMeshComponent->bUseAsyncCooking = false;
	FillMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FillMeshComponent->SetCastShadow(false);

	UnderwaterComponent = CreateDefaultSubobject<UVoxelUnderwaterNetworkComponent>(TEXT("UnderwaterNetwork"));

	Tags.Add(TEXT("VoxelSplineNetwork"));
}

void AVoxelSplineNetworkGenerator::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [BeginPlay] Actor='%s' bGenerateOnBeginPlay=%s GenerationDelay=%.2f"),
		*GetName(), bGenerateOnBeginPlay ? TEXT("true") : TEXT("false"), GenerationDelay);
	
	if (bGenerateOnBeginPlay)
	{
		GenerateNetworkDelayed();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [BeginPlay] bGenerateOnBeginPlay is FALSE — not generating. Check the checkbox on the placed actor instance, not just the class default."));
	}
}

void AVoxelSplineNetworkGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bBillboardLabels) return;
	APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!CM) return;
	const FVector CamLoc = CM->GetCameraLocation();

	auto BillboardLabels = [&](const TArray<TObjectPtr<UTextRenderComponent>>& Labels)
	{
		for (UTextRenderComponent* Label : Labels)
		{
			if (!Label) continue;
			const FVector Away = (Label->GetComponentLocation() - CamLoc).GetSafeNormal();
			Label->SetWorldRotation(FRotator(0.f, Away.Rotation().Yaw + 180.f, 0.f));
		}
	};

	BillboardLabels(LabelComponents);
	if (UnderwaterComponent) BillboardLabels(UnderwaterComponent->GetLabelComponents());
}

void AVoxelSplineNetworkGenerator::Destroyed()
{
	if (GenerateDelayHandle.IsValid()) GetWorldTimerManager().ClearTimer(GenerateDelayHandle);
	if (bVoxelWorldLODForced)
	{
		if (AVoxelWorld* VW = ResolveVoxelWorld())
		{
			RestoreVoxelWorldLOD(VW);
		}
	}
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
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bBillboardLabels),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowDocks),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, DockDebugColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, DockDebugThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowRoads),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, RoadDebugColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, RoadDebugThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowSeaLanes),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, SeaLaneDebugColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, SeaLaneDebugThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowCoast),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, CoastDebugColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, CoastDebugThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, DebugDuration),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, DebugZLift),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowUnderwaterEdges),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, UnderwaterEdgeColor),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, UnderwaterEdgeThickness),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowUnderwaterLabels),
		GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, bShowUnderwaterBiomeLabelOnAllCells),
	};
	if (DebugOnly.Contains(Prop)) RefreshDebugDraw();

	if (Prop == GET_MEMBER_NAME_CHECKED(AVoxelSplineNetworkGenerator, NetworkWidth) && bAutoSizeFromVoxelWorld)
	{
		const FString Msg = TEXT("VoxelSplineNetworkGenerator: 'Network Width' has NO effect while 'Auto-Size From Voxel World' is enabled — it is not read at all. Uncheck Auto-Size to use a manual width, or use ProbeTerrainExtent()/the log output to see what width auto-size actually detects.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow, Msg);
	}
}
#endif

void AVoxelSplineNetworkGenerator::ClearNetwork()
{
	if (GenerateDelayHandle.IsValid()) GetWorldTimerManager().ClearTimer(GenerateDelayHandle);
	
	Cells.Empty();
	CachedTriangles.Empty();
	CachedHeightGrid.Empty();
	CachedBiomeGrid.Empty();
	CachedGridRes = 0;
	BiomeRowNames.Empty();    BiomeRows.Empty();
	LocationRowNames.Empty(); LocationRows.Empty();
	IslandDockCellMap.Empty();
	GenericDockCellSet.Empty();
	DockShorelinePointMap.Empty();
	ForcePlacedBiomeCells.Empty();

	for (USplineComponent* S : BiomeSplines)   if (S) S->DestroyComponent();
	for (USplineComponent* S : CentreSplines)  if (S) S->DestroyComponent();
	for (USplineComponent* S : DockSplines)    if (S) S->DestroyComponent();
	for (USplineComponent* S : RoadSplines)    if (S) S->DestroyComponent();
	for (USplineComponent* S : SeaLaneSplines) if (S) S->DestroyComponent();
	for (USplineComponent* S : CoastSplines)   if (S) S->DestroyComponent();
	BiomeSplines.Empty(); CentreSplines.Empty(); DockSplines.Empty();
	RoadSplines.Empty(); SeaLaneSplines.Empty(); CoastSplines.Empty();
	
	if (UnderwaterComponent) UnderwaterComponent->Clear();

	if (FillMeshComponent) FillMeshComponent->ClearAllMeshSections();
	ClearLabelComponents();
	FlushPersistentDebugLines(GetWorld());
}

void AVoxelSplineNetworkGenerator::ClearNetworkRuntime() { ClearNetwork(); }

void AVoxelSplineNetworkGenerator::GenerateNetwork()
{
	UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [GenerateNetwork] Called (instant, no delay) on Actor='%s'."), *GetName());
	if (GenerateDelayHandle.IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("VoxelSplineNetworkGenerator: [GenerateNetwork] A delayed generation (started by GenerateNetworkDelayed() / bGenerateOnBeginPlay) was still pending — cancelling it now. "
			     "GenerateNetwork() always runs instantly and ignores GenerationDelay entirely. If you didn't call GenerateNetwork() yourself just now, something else did — check this actor's own "
			     "Blueprint Event Graph (a duplicate 'Event BeginPlay -> Generate Network' is the usual culprit) or any other actor/Level Blueprint calling it, and remove that call so GenerationDelay is actually respected."));
		GetWorldTimerManager().ClearTimer(GenerateDelayHandle);
	}
	if (bForceLOD0CollisionBeforeGeneration)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [GenerateNetwork] bForceLOD0CollisionBeforeGeneration is on, but this is the INSTANT path — there is no delay window for the async chunk/collision rebuild to finish before tracing runs below. Prefer GenerateNetworkDelayed()/bGenerateOnBeginPlay so it actually has time to take effect."));
		if (AVoxelWorld* VW = ResolveVoxelWorld())
		{
			ForceCollisionAcrossVoxelWorld(VW);
		}
	}
	ClearNetwork();
	RunGenerationPipeline();
}

void AVoxelSplineNetworkGenerator::GenerateNetworkDelayed(float DelaySeconds)
{
	const float Delay = (DelaySeconds >= 0.f) ? DelaySeconds : GenerationDelay;
	ClearNetwork();
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [GenerateNetworkDelayed] Actor='%s' World='%s' HasAuthority=%s DelaySeconds(param)=%.2f GenerationDelay(field)=%.2f -> using Delay=%.2f"),
		*GetName(),
		GetWorld() ? *GetWorld()->GetName() : TEXT("NULL"),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		DelaySeconds, GenerationDelay, Delay);

	if (bForceLOD0CollisionBeforeGeneration)
	{
		if (AVoxelWorld* VW = ResolveVoxelWorld())
		{
			ForceCollisionAcrossVoxelWorld(VW);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [GenerateNetworkDelayed] bForceLOD0CollisionBeforeGeneration is true but no VoxelWorld could be resolved yet — nothing to pin. If the VoxelWorld spawns after this actor, this may just be a timing issue."));
		}
	}

	if (Delay <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [GenerateNetworkDelayed] Delay <= 0, running pipeline immediately."));
		RunGenerationPipeline();
		return;
	}
	GetWorldTimerManager().SetTimer(GenerateDelayHandle, this,
		&AVoxelSplineNetworkGenerator::RunGenerationPipeline, Delay, false);
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [GenerateNetworkDelayed] Timer set. TimerHandle valid=%s. Pipeline will run in %.2fs."),
		GenerateDelayHandle.IsValid() ? TEXT("true") : TEXT("false"), Delay);
}

float AVoxelSplineNetworkGenerator::ComputeNetworkWidth() const
{
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkWidth] bAutoSizeFromVoxelWorld=%s ManualNetworkWidth=%.0f"),
		bAutoSizeFromVoxelWorld ? TEXT("true") : TEXT("false"), NetworkWidth);

	if (!bAutoSizeFromVoxelWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkWidth] Auto-size OFF — returning manual NetworkWidth=%.0f"), NetworkWidth);
		return NetworkWidth;
	}

	AVoxelWorld* VW = ResolveVoxelWorld();
	if (!VW)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkWidth] ResolveVoxelWorld() returned NULL — no AVoxelWorld assigned and none found in the level. Using NetworkWidth %.0f."), NetworkWidth);
		return NetworkWidth;
	}
	UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkWidth] Resolved VoxelWorld='%s' at Location=(%.0f, %.0f, %.0f)"),
		*VW->GetName(), VW->GetActorLocation().X, VW->GetActorLocation().Y, VW->GetActorLocation().Z);
	
	FBox2D ComponentBounds2D;
	float RenderFootprintWidth = 0.f;
	const bool bHaveRenderFootprint = ComputeFilteredActorBounds2D(VW, ComponentBounds2D);
	if (bHaveRenderFootprint)
	{
		const FVector2D Size = ComponentBounds2D.GetSize();
		RenderFootprintWidth = FMath::Max(Size.X, Size.Y);
	}
	
	const FVector2D ProbeOrigin(VW->GetActorLocation());
	float ProbedRadius = 0.f;
	const bool bFoundTerrain = ProbeTerrainRadius(ProbeOrigin, ProbedRadius, /*bLandOnly=*/false);

	if (bHaveRenderFootprint)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkWidth] Render footprint (from mesh bounds) = %.0f wide. Collision probe = %.0f wide. %s"),
			RenderFootprintWidth, bFoundTerrain ? ProbedRadius * 2.f : 0.f,
			(bFoundTerrain && RenderFootprintWidth > ProbedRadius * 4.f)
				? TEXT("Render footprint is dramatically bigger than what collision can reach — this plugin's LOD chunks past the collision radius have no collision, exactly as suspected.")
				: TEXT(""));
	}

	if (!bFoundTerrain || ProbedRadius < 500.f)
	{
		UE_LOG(LogTemp, Error,
			TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkWidth] Probing outward from (%.0f, %.0f) found no terrain within range (%d directions x %d steps x %.0f step = max radius %.0f). "
			     "Falling back to manual NetworkWidth %.0f. Common causes: GenerationDelay too short (terrain collision not cooked yet when this ran), the VoxelWorld actor isn't actually where the terrain is, "
			     "or TraceStartZ/TraceEndZ don't bracket the terrain's Z range."),
			ProbeOrigin.X, ProbeOrigin.Y, AutoSizeProbeDirections, AutoSizeMaxProbeSteps, AutoSizeProbeStep,
			AutoSizeProbeStep * AutoSizeMaxProbeSteps, NetworkWidth);
		return NetworkWidth;
	}

	const float Detected = ProbedRadius * 2.f * AutoSizePadding;
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkWidth] Probed terrain radius=%.0f (padding x%.2f) -> auto-detected width=%.0f. Using this (collision-based) for the network."),
		ProbedRadius, AutoSizePadding, Detected);
	return Detected;
}

bool AVoxelSplineNetworkGenerator::ComputeFilteredActorBounds2D(AActor* Actor, FBox2D& OutBounds2D) const
{
	if (!Actor) return false;

	TArray<UPrimitiveComponent*> Prims;
	Actor->GetComponents<UPrimitiveComponent>(Prims);

	struct FCompInfo
	{
		FString Name, ClassName;
		float   MaxExtent = 0.f;
		FVector Origin, Extent;
		bool    bExcludedByClass = false;
		bool    bExcludedBySize  = false;
	};
	TArray<FCompInfo> Infos;
	Infos.Reserve(Prims.Num());

	for (UPrimitiveComponent* P : Prims)
	{
		if (!P || !P->IsRegistered()) continue;
		const FBoxSphereBounds B = P->Bounds;
		if (B.BoxExtent.GetMax() <= KINDA_SMALL_NUMBER) continue;

		FCompInfo Info;
		Info.Name      = P->GetName();
		Info.ClassName = P->GetClass()->GetName();
		Info.MaxExtent = B.BoxExtent.GetMax();
		Info.Origin    = B.Origin;
		Info.Extent    = B.BoxExtent;

		for (const FString& Substr : AutoSizeExcludedComponentClassSubstrings)
		{
			if (!Substr.IsEmpty() && Info.ClassName.Contains(Substr, ESearchCase::IgnoreCase))
			{
				Info.bExcludedByClass = true;
				break;
			}
		}
		Info.bExcludedBySize = Info.MaxExtent > AutoSizeMaxPlausibleComponentExtent;

		Infos.Add(MoveTemp(Info));
	}
	
	Infos.Sort([](const FCompInfo& A, const FCompInfo& B) { return A.MaxExtent > B.MaxExtent; });
	const int32 DumpCount = FMath::Min(Infos.Num(), 12);
	UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [ComputeFilteredActorBounds2D] %d non-empty primitive component(s) on '%s'. Top %d by size:"),
		Infos.Num(), *Actor->GetName(), DumpCount);
	for (int32 i = 0; i < DumpCount; ++i)
	{
		const FCompInfo& Info = Infos[i];
		const TCHAR* Status = Info.bExcludedByClass ? TEXT("EXCLUDED-CLASS") : Info.bExcludedBySize ? TEXT("EXCLUDED-SIZE") : TEXT("included");
		UE_LOG(LogTemp, Warning, TEXT("  [%s] '%s' (class '%s') Extent=%.0f Origin=(%.0f,%.0f,%.0f)"),
			Status, *Info.Name, *Info.ClassName, Info.MaxExtent, Info.Origin.X, Info.Origin.Y, Info.Origin.Z);
	}

	bool bFoundAny = false;
	FVector2D MinP( TNumericLimits<float>::Max(),  TNumericLimits<float>::Max());
	FVector2D MaxP(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());

	for (const FCompInfo& Info : Infos)
	{
		if (Info.bExcludedByClass || Info.bExcludedBySize) continue;
		bFoundAny = true;
		MinP.X = FMath::Min(MinP.X, Info.Origin.X - Info.Extent.X); MaxP.X = FMath::Max(MaxP.X, Info.Origin.X + Info.Extent.X);
		MinP.Y = FMath::Min(MinP.Y, Info.Origin.Y - Info.Extent.Y); MaxP.Y = FMath::Max(MaxP.Y, Info.Origin.Y + Info.Extent.Y);
	}

	if (!bFoundAny) return false;
	OutBounds2D = FBox2D(MinP, MaxP);
	return true;
}

void AVoxelSplineNetworkGenerator::ForceCollisionAcrossVoxelWorld(AVoxelWorld* VW)
{
	if (!VW || bVoxelWorldLODForced) return;

	SavedVoxelWorldMaxLOD       = VW->MaxLOD;
	SavedVoxelWorldMinLOD       = VW->MinLOD;
	bSavedVoxelWorldConstantLOD = VW->bConstantLOD;

	VW->MaxLOD       = 0;
	VW->MinLOD       = 0;
	VW->bConstantLOD = true;
	VW->UpdateDynamicLODSettings();
	VW->UpdateCollisionProfile();

	bVoxelWorldLODForced = true;

	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [ForceCollisionAcrossVoxelWorld] Pinned '%s' to LOD0 (was MaxLOD=%d MinLOD=%d ConstantLOD=%s) and called UpdateDynamicLODSettings() + UpdateCollisionProfile(). "
		     "This is async — real chunk/collision rebuild takes time — so GenerationDelay needs to be long enough after this call for it to actually finish before tracing runs."),
		*VW->GetName(), SavedVoxelWorldMaxLOD, SavedVoxelWorldMinLOD, bSavedVoxelWorldConstantLOD ? TEXT("true") : TEXT("false"));
}

void AVoxelSplineNetworkGenerator::RestoreVoxelWorldLOD(AVoxelWorld* VW)
{
	if (!VW || !bVoxelWorldLODForced) return;

	VW->MaxLOD       = SavedVoxelWorldMaxLOD;
	VW->MinLOD       = SavedVoxelWorldMinLOD;
	VW->bConstantLOD = bSavedVoxelWorldConstantLOD;
	VW->UpdateDynamicLODSettings();

	bVoxelWorldLODForced = false;

	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [RestoreVoxelWorldLOD] Restored '%s' LOD settings (MaxLOD=%d MinLOD=%d ConstantLOD=%s)."),
		*VW->GetName(), SavedVoxelWorldMaxLOD, SavedVoxelWorldMinLOD, bSavedVoxelWorldConstantLOD ? TEXT("true") : TEXT("false"));
}

bool AVoxelSplineNetworkGenerator::ProbeTerrainRadius(const FVector2D& Origin, float& OutMaxRadius, bool bLandOnly) const
{
	const int32 NumDirections = FMath::Max(AutoSizeProbeDirections, 4);
	const float StepSize      = FMath::Max(AutoSizeProbeStep, 100.f);
	const int32 MaxSteps      = FMath::Max(AutoSizeMaxProbeSteps, 1);

	OutMaxRadius = 0.f;
	bool bFoundAny = false;
	
	for (int32 D = 0; D < NumDirections; ++D)
	{
		const float Angle = (2.f * PI * D) / NumDirections;
		const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));

		for (int32 Step = 1; Step <= MaxSteps; ++Step)
		{
			const FVector2D P = Origin + Dir * (StepSize * Step);
			FVector Hit;
			const bool bHit = TraceGroundAt(P.X, P.Y, Hit) && (!bLandOnly || Hit.Z > CoastHeightThreshold);

			if (bHit)
			{
				bFoundAny = true;
				OutMaxRadius = FMath::Max(OutMaxRadius, (P - Origin).Size());
			}
		}
	}

	return bFoundAny;
}

float AVoxelSplineNetworkGenerator::ProbeTerrainExtent(bool bLandOnly) const
{
	AVoxelWorld* VW = ResolveVoxelWorld();
	const FVector2D Origin = VW ? FVector2D(VW->GetActorLocation()) : FVector2D(GetActorLocation());

	float Radius = 0.f;
	const bool bFound = ProbeTerrainRadius(Origin, Radius, bLandOnly);
	const float Width = bFound ? Radius * 2.f : 0.f;

	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [ProbeTerrainExtent] bLandOnly=%s Origin=(%.0f, %.0f) -> %s. Radius=%.0f Width=%.0f"),
		bLandOnly ? TEXT("true") : TEXT("false"), Origin.X, Origin.Y,
		bFound ? TEXT("FOUND") : TEXT("NOTHING FOUND"), Radius, Width);

	return Width;
}

FVector2D AVoxelSplineNetworkGenerator::ComputeNetworkOrigin() const
{
	AVoxelWorld* VW = ResolveVoxelWorld();
	if (VW)
	{
		const FVector2D Result(VW->GetActorLocation());
		UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkOrigin] Using VoxelWorld '%s' location = (%.0f, %.0f)"),
			*VW->GetName(), Result.X, Result.Y);
		return Result;
	}

	const FVector2D Fallback(GetActorLocation());
	UE_LOG(LogTemp, Error,
		TEXT("VoxelSplineNetworkGenerator: [ComputeNetworkOrigin] No AVoxelWorld — centring network on generator actor's own location (%.0f, %.0f) as a last resort. This will likely be WRONG if the generator isn't placed exactly at the terrain centre — assign VoxelWorld explicitly on the generator!"),
		Fallback.X, Fallback.Y);
	return Fallback;
}

float AVoxelSplineNetworkGenerator::SafeRoadTerrainZ(float X, float Y, float FallbackZ) const
{
	const float WaterFloor = CoastHeightThreshold + RoadZOffset;
	FVector Hit;
	if (TraceGroundAt(X, Y, Hit))
		return FMath::Max(FMath::Max(Hit.Z, MinRoadHeight), WaterFloor);
	return FMath::Max(FMath::Max(FallbackZ, MinRoadHeight), WaterFloor);
}

float AVoxelSplineNetworkGenerator::DockRadiusForCell(int32 CellIndex) const
{
	if (GenericDockCellSet.Contains(CellIndex)) return GenericDockRadius;
	if (Cells.IsValidIndex(CellIndex) && Cells[CellIndex].LocationRowName != NAME_None)
	{
		const int32 Li = LocationRowNames.IndexOfByKey(Cells[CellIndex].LocationRowName);
		if (LocationRows.IsValidIndex(Li)) return LocationRows[Li].CentreRadius;
	}
	return 0.f;
}

bool AVoxelSplineNetworkGenerator::IsRegisteredDockCell(int32 Ci) const
{
	if (GenericDockCellSet.Contains(Ci)) return true;
	if (!Cells.IsValidIndex(Ci)) return false;
	const int32 Li = LocationRowNames.IndexOfByKey(Cells[Ci].LocationRowName);
	return LocationRows.IsValidIndex(Li) && LocationRows[Li].bConnectsToSeaLane;
}

void AVoxelSplineNetworkGenerator::RunGenerationPipeline()
{
	ComputedNetworkWidth  = ComputeNetworkWidth();
	ComputedNetworkOrigin = ComputeNetworkOrigin();

	{
		const FBox2D DebugBounds = GetNetworkBounds2D();
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelSplineNetworkGenerator: [RunGenerationPipeline] Final network bounds: Min=(%.0f, %.0f) Max=(%.0f, %.0f) (Origin=(%.0f, %.0f), Width=%.0f)"),
			DebugBounds.Min.X, DebugBounds.Min.Y, DebugBounds.Max.X, DebugBounds.Max.Y,
			ComputedNetworkOrigin.X, ComputedNetworkOrigin.Y, ComputedNetworkWidth);
	}

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

	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: width=%.0f, %d biomes, %d locations."),
		ComputedNetworkWidth, BiomeRows.Num(), LocationRows.Num());

	GenerateCellPoints();
	BuildDelaunayGraph();
	ComputeVoronoiPolygons();
	AssignBiomes();
	TraceTerrainHeights();
	ComputeIslandIndices();
	AssignLocations();

	if (bGenerateSeaLanes) GatherDockCells();

	SampleHeightGrid();
	RasterizeBiomeGrid();
	BuildCellSplines();
	BuildCentreSplines();
	BuildDockSplines();
	BuildRoadSplines();
	if (bGenerateSeaLanes)     BuildSeaLaneSplines();
	if (bGenerateCoastSplines) BuildCoastlineSplines();

	if (bGenerateUnderwaterNetwork && UnderwaterComponent) UnderwaterComponent->RunPipeline();

	RefreshDebugDraw();

	int32 LandCells = 0;
	for (const FVoxelBiomeCell& C : Cells) if (C.bOnTerrain) ++LandCells;
	UE_LOG(LogTemp, Log,
		TEXT("VoxelSplineNetworkGenerator: Done. %d cells (%d land), %d biomes, %d centres, "
		     "%d docks (%d generic), %d roads, %d sea lanes, %d coast."),
		Cells.Num(), LandCells, BiomeSplines.Num(), CentreSplines.Num(),
		DockSplines.Num(), GenericDockCellSet.Num(),
		RoadSplines.Num(), SeaLaneSplines.Num(), CoastSplines.Num());

	if (bForceLOD0CollisionBeforeGeneration && bRestoreLODAfterGeneration)
	{
		if (AVoxelWorld* VW = ResolveVoxelWorld())
		{
			RestoreVoxelWorldLOD(VW);
		}
	}
}

void AVoxelSplineNetworkGenerator::RefreshDebugDraw()
{
	FlushPersistentDebugLines(GetWorld());
	RebuildFillMesh();
	if (bShowEdges)           DrawDebugEdges();
	DrawDebugLocationMarkers();
	ClearLabelComponents();
	if (bShowLabels)
	{
		BuildLabelComponents();
		BuildDockLabelComponents();
	}
	if (bShowDocks)              DrawDebugDocks();
	if (bShowRoads)              DrawDebugRoads();
	if (bShowSeaLanes)           DrawDebugSeaLanes();
	if (bShowCoast)              DrawDebugCoast();

	if (UnderwaterComponent) UnderwaterComponent->RefreshDebugDraw();
}

FString AVoxelSplineNetworkGenerator::SanitiseName(const FString& In)
{
	FString Out = In;
	for (const TCHAR Bad : { TEXT(' '), TEXT('/'), TEXT('\\'), TEXT('.') })
		Out.ReplaceCharInline(Bad, TEXT('_'), ESearchCase::CaseSensitive);
	return Out;
}

namespace VoxelRowReflection
{
	static const FProperty* FindPropertyFuzzy(const UScriptStruct* S, FName Field)
	{
		if (const FProperty* P = S->FindPropertyByName(Field))
			return P;

		const FString FieldStr = Field.ToString();
		for (TFieldIterator<FProperty> It(S); It; ++It)
		{
			const FString PropName = It->GetName();
			if (PropName.Equals(FieldStr, ESearchCase::IgnoreCase))
				return *It;
			if (PropName.StartsWith(FieldStr + TEXT("_"), ESearchCase::IgnoreCase))
				return *It;
		}
		return nullptr;
	}

	static FString ReadRowString(const UScriptStruct* S, const void* Row, FName Field, const FString& Def = TEXT(""))
	{
		if (const FProperty* P = FindPropertyFuzzy(S, Field))
		{
			if (const FStrProperty* SP = CastField<FStrProperty>(P))
				return SP->GetPropertyValue_InContainer(Row);
			if (const FNameProperty* NP = CastField<FNameProperty>(P))
				return NP->GetPropertyValue_InContainer(Row).ToString();
			if (const FTextProperty* TP = CastField<FTextProperty>(P))
				return TP->GetPropertyValue_InContainer(Row).ToString();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: Row struct '%s' has no field '%s' (String)."),
				*S->GetName(), *Field.ToString());
		}
		return Def;
	}

	static FName ReadRowName(const UScriptStruct* S, const void* Row, FName Field, FName Def = NAME_None)
	{
		if (const FProperty* P = FindPropertyFuzzy(S, Field))
		{
			if (const FNameProperty* NP = CastField<FNameProperty>(P))
				return NP->GetPropertyValue_InContainer(Row);
			if (const FStrProperty* SP = CastField<FStrProperty>(P))
				return FName(*SP->GetPropertyValue_InContainer(Row));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: Row struct '%s' has no field '%s' (Name)."),
				*S->GetName(), *Field.ToString());
		}
		return Def;
	}

	static float ReadRowFloat(const UScriptStruct* S, const void* Row, FName Field, float Def = 0.f)
	{
		if (const FProperty* P = FindPropertyFuzzy(S, Field))
		{
			if (const FFloatProperty* FP = CastField<FFloatProperty>(P))
				return FP->GetPropertyValue_InContainer(Row);
			if (const FDoubleProperty* DP = CastField<FDoubleProperty>(P))
				return (float)DP->GetPropertyValue_InContainer(Row);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: Row struct '%s' has no field '%s' (Float)."),
				*S->GetName(), *Field.ToString());
		}
		return Def;
	}

	static bool ReadRowBool(const UScriptStruct* S, const void* Row, FName Field, bool Def = false)
	{
		if (const FProperty* P = FindPropertyFuzzy(S, Field))
		{
			if (const FBoolProperty* BP = CastField<FBoolProperty>(P))
				return BP->GetPropertyValue_InContainer(Row);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: Row struct '%s' has no field '%s' (Bool)."),
				*S->GetName(), *Field.ToString());
		}
		return Def;
	}

	static FLinearColor ReadRowLinearColor(const UScriptStruct* S, const void* Row, FName Field,
	                                    FLinearColor Def = FLinearColor::White)
	{
		if (const FProperty* P = FindPropertyFuzzy(S, Field))
		{
			if (const FStructProperty* StP = CastField<FStructProperty>(P))
			{
				if (StP->Struct == TBaseStructure<FLinearColor>::Get())
				{
					const void* ValuePtr = StP->ContainerPtrToValuePtr<void>(Row);
					return *reinterpret_cast<const FLinearColor*>(ValuePtr);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: Row struct '%s' has no field '%s' (LinearColor)."),
				*S->GetName(), *Field.ToString());
		}
		return Def;
	}

	static TArray<FName> ReadRowNameArray(const UScriptStruct* S, const void* Row, FName Field)
	{
		TArray<FName> Out;
		if (const FProperty* P = FindPropertyFuzzy(S, Field))
		{
			if (const FArrayProperty* AP = CastField<FArrayProperty>(P))
			{
				const void* ArrPtr = AP->ContainerPtrToValuePtr<void>(Row);
				FScriptArrayHelper Helper(AP, ArrPtr);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					if (const FNameProperty* InnerNP = CastField<FNameProperty>(AP->Inner))
						Out.Add(InnerNP->GetPropertyValue(Helper.GetRawPtr(i)));
					else if (const FStrProperty* InnerSP = CastField<FStrProperty>(AP->Inner))
						Out.Add(FName(*InnerSP->GetPropertyValue(Helper.GetRawPtr(i))));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: Row struct '%s' has no field '%s' (Name array)."),
				*S->GetName(), *Field.ToString());
		}
		return Out;
	}
}

void AVoxelSplineNetworkGenerator::LoadDataTables()
{
	using namespace VoxelRowReflection;

	BiomeRowNames.Empty(); BiomeRows.Empty();
	LocationRowNames.Empty(); LocationRows.Empty();

	if (BiomeDataTable)
	{
		const UScriptStruct* RowStruct = BiomeDataTable->GetRowStruct();
		if (RowStruct)
		{
			for (const FName& Key : BiomeDataTable->GetRowNames())
			{
				const uint8* RowPtr = BiomeDataTable->FindRowUnchecked(Key);
				if (!RowPtr) continue;

				FVoxelBiomeRow Row;
				Row.BiomeName       = ReadRowString(RowStruct, RowPtr, TEXT("BiomeName"));
				Row.PCGTag          = ReadRowName  (RowStruct, RowPtr, TEXT("PCGTag"));
				Row.DebugColor      = ReadRowLinearColor(RowStruct, RowPtr, TEXT("DebugColor"));
				Row.CanBorderBiomes = ReadRowNameArray(RowStruct, RowPtr, TEXT("CanBorderBiomes"));

				BiomeRowNames.Add(Key);
				BiomeRows.Add(Row);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: BiomeDataTable has no row struct set."));
		}
	}

	if (BiomeRows.IsEmpty() && bUseExampleDataIfMissing)
	{
		LoadExampleBiomeRows(BiomeRowNames, BiomeRows);
		UE_LOG(LogTemp, Log,
			TEXT("VoxelSplineNetworkGenerator: BiomeDataTable missing — using %d example biome row(s)."),
			BiomeRows.Num());
	}

	if (LocationDataTable)
	{
		const UScriptStruct* RowStruct = LocationDataTable->GetRowStruct();
		if (RowStruct)
		{
			for (const FName& Key : LocationDataTable->GetRowNames())
			{
				const uint8* RowPtr = LocationDataTable->FindRowUnchecked(Key);
				if (!RowPtr) continue;

				FVoxelLocationRow Row;
				Row.DisplayName            = ReadRowString(RowStruct, RowPtr, TEXT("DisplayName"));
				Row.PCGTag                 = ReadRowName  (RowStruct, RowPtr, TEXT("PCGTag"));
				Row.CategoryTag            = ReadRowName  (RowStruct, RowPtr, TEXT("CategoryTag"));
				Row.AllowedBiomes          = ReadRowNameArray(RowStruct, RowPtr, TEXT("AllowedBiomes"));
				Row.CentreRadius           = ReadRowFloat (RowStruct, RowPtr, TEXT("CentreRadius"));
				Row.MinSpacingFromOthers   = ReadRowFloat (RowStruct, RowPtr, TEXT("MinSpacingFromOthers"));
				Row.DebugColor             = ReadRowLinearColor(RowStruct, RowPtr, TEXT("DebugColor"));
				Row.bConnectsToRoadNetwork = ReadRowBool  (RowStruct, RowPtr, TEXT("bConnectsToRoadNetwork"));
				Row.bConnectsToSeaLane     = ReadRowBool  (RowStruct, RowPtr, TEXT("bConnectsToSeaLane"));

				LocationRowNames.Add(Key);
				LocationRows.Add(Row);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: LocationDataTable has no row struct set."));
		}
	}

	if (LocationRows.IsEmpty() && bUseExampleDataIfMissing)
	{
		LoadExampleLocationRows(LocationRowNames, LocationRows);
		UE_LOG(LogTemp, Log,
			TEXT("VoxelSplineNetworkGenerator: LocationDataTable missing — using %d example location row(s)."),
			LocationRows.Num());
	}
}

void AVoxelSplineNetworkGenerator::LoadExampleBiomeRows(
	TArray<FName>& OutNames, TArray<FVoxelBiomeRow>& OutRows) const
{
	OutNames.Empty(); OutRows.Empty();

	auto Add = [&](FName Name, const FString& DisplayName, FName PCGTag, FLinearColor Colour,
	               std::initializer_list<FName> CanBorder)
	{
		FVoxelBiomeRow Row;
		Row.BiomeName       = DisplayName;
		Row.PCGTag          = PCGTag;
		Row.DebugColor      = Colour;
		Row.CanBorderBiomes = CanBorder;
		OutNames.Add(Name); OutRows.Add(Row);
	};

	Add(TEXT("Forest"),    TEXT("Forest"),    TEXT("PCG_Biome_Forest"),    FLinearColor(0.10f,0.45f,0.12f), {TEXT("Plains"),TEXT("Mountains"),TEXT("Swamp")});
	Add(TEXT("Plains"),    TEXT("Plains"),    TEXT("PCG_Biome_Plains"),    FLinearColor(0.55f,0.65f,0.20f), {TEXT("Forest"),TEXT("Desert"),TEXT("Swamp"),TEXT("Mountains")});
	Add(TEXT("Mountains"), TEXT("Mountains"), TEXT("PCG_Biome_Mountains"), FLinearColor(0.45f,0.45f,0.48f), {TEXT("Forest"),TEXT("Plains")});
	Add(TEXT("Desert"),    TEXT("Desert"),    TEXT("PCG_Biome_Desert"),    FLinearColor(0.80f,0.65f,0.30f), {TEXT("Plains")});
	Add(TEXT("Swamp"),     TEXT("Swamp"),     TEXT("PCG_Biome_Swamp"),     FLinearColor(0.25f,0.30f,0.15f), {TEXT("Forest"),TEXT("Plains")});
}

void AVoxelSplineNetworkGenerator::LoadExampleLocationRows(
	TArray<FName>& OutNames, TArray<FVoxelLocationRow>& OutRows) const
{
	OutNames.Empty(); OutRows.Empty();

	auto Add = [&](FName Name, const FString& DisplayName, FName PCGTag, FName Category,
	               std::initializer_list<FName> Allowed, float Radius, float Spacing,
	               bool bRoad, bool bSeaLane)
	{
		FVoxelLocationRow Row;
		Row.DisplayName            = DisplayName;
		Row.PCGTag                 = PCGTag;
		Row.CategoryTag            = Category;
		Row.AllowedBiomes          = Allowed;
		Row.CentreRadius           = Radius;
		Row.MinSpacingFromOthers   = Spacing;
		Row.bConnectsToRoadNetwork = bRoad;
		Row.bConnectsToSeaLane     = bSeaLane;
		OutNames.Add(Name); OutRows.Add(Row);
	};

	Add(TEXT("CapitalCity"), TEXT("Capital City"), TEXT("PCG_Loc_CapitalCity"), TEXT("PCG_City"),
		{TEXT("Plains")}, 15000.f, 150000.f, true, true);
	Add(TEXT("Village"), TEXT("Village"), TEXT("PCG_Loc_Village"), TEXT("PCG_City"),
		{TEXT("Forest"),TEXT("Plains")}, 6000.f, 60000.f, true, false);
	Add(TEXT("Watchtower"), TEXT("Watchtower"), TEXT("PCG_Loc_Watchtower"), TEXT("PCG_Landmark"),
		{TEXT("Mountains")}, 3000.f, 40000.f, true, false);
}

void AVoxelSplineNetworkGenerator::GenerateCellPoints()
{
	FRandomStream Rand(Seed);
	const int32 GridDim     = FMath::CeilToInt(FMath::Sqrt((float)CellCount));
	const float CellSpacing = ComputedNetworkWidth / FMath::Max(GridDim - 1, 1);
	const float HalfWidth   = ComputedNetworkWidth * 0.5f;
	const float JitterMax   = CellSpacing * JitterAmount * 0.5f;
	const FVector2D Origin  = ComputedNetworkOrigin;

	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [GenerateCellPoints] Origin=(%.0f, %.0f) ComputedNetworkWidth=%.0f GridDim=%d CellSpacing=%.1f HalfWidth=%.0f JitterMax=%.1f CellCount(requested)=%d"),
		Origin.X, Origin.Y, ComputedNetworkWidth, GridDim, CellSpacing, HalfWidth, JitterMax, CellCount);

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

	if (Cells.Num() > 0)
	{
		FVector2D MinP(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		FVector2D MaxP(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
		for (const FVoxelBiomeCell& C : Cells)
		{
			MinP.X = FMath::Min(MinP.X, C.WorldPosition.X); MaxP.X = FMath::Max(MaxP.X, C.WorldPosition.X);
			MinP.Y = FMath::Min(MinP.Y, C.WorldPosition.Y); MaxP.Y = FMath::Max(MaxP.Y, C.WorldPosition.Y);
		}
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelSplineNetworkGenerator: [GenerateCellPoints] Generated %d cells. Actual XY extent: Min=(%.0f, %.0f) Max=(%.0f, %.0f)"),
			Cells.Num(), MinP.X, MinP.Y, MaxP.X, MaxP.Y);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelSplineNetworkGenerator: [GenerateCellPoints] Generated 0 cells! GridDim=%d CellCount=%d"), GridDim, CellCount);
	}
}

void AVoxelSplineNetworkGenerator::BuildDelaunayGraph()
{
	if (Cells.Num() < 3)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelSplineNetworkGenerator: [BuildDelaunayGraph] Only %d cells — need at least 3, skipping."), Cells.Num());
		return;
	}

	TArray<FVector2D> Points;
	Points.Reserve(Cells.Num());
	for (const FVoxelBiomeCell& C : Cells)
		Points.Add(FVector2D(C.WorldPosition.X, C.WorldPosition.Y));

	CachedTriangles = FVoxelDelaunayVoronoiUtils::BowyerWatsonFromPoints(Points);
	for (const FDelaunayTriangle& T : CachedTriangles)
	{
		auto Link = [&](int32 A, int32 B)
		{ if (Cells.IsValidIndex(A) && Cells.IsValidIndex(B)) Cells[A].NeighborIndices.AddUnique(B); };
		Link(T.A,T.B); Link(T.B,T.A); Link(T.B,T.C); Link(T.C,T.B); Link(T.A,T.C); Link(T.C,T.A);
	}
	UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: [BuildDelaunayGraph] %d points -> %d triangles."), Points.Num(), CachedTriangles.Num());
}

FBox2D AVoxelSplineNetworkGenerator::GetNetworkBounds2D() const
{
	const FVector2D O = ComputedNetworkOrigin;
	const float H = ComputedNetworkWidth * .5f;
	return FBox2D(FVector2D(O.X-H, O.Y-H), FVector2D(O.X+H, O.Y+H));
}

void AVoxelSplineNetworkGenerator::ComputeVoronoiPolygons()
{
	TArray<FVector2D> Positions;
	Positions.Reserve(Cells.Num());
	for (const FVoxelBiomeCell& C : Cells)
		Positions.Add(FVector2D(C.WorldPosition.X, C.WorldPosition.Y));

	const FBox2D Bounds = GetNetworkBounds2D();
	int32 EmptyPolyCount = 0;
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		Cells[i].VoronoiPolygon2D =
			FVoxelDelaunayVoronoiUtils::ComputeVoronoiPolygon2D(i, Positions, CachedTriangles, Bounds);
		if (Cells[i].VoronoiPolygon2D.Num() == 0) ++EmptyPolyCount;
	}
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [ComputeVoronoiPolygons] %d cells processed, %d have EMPTY polygons (likely clipped entirely outside bounds Min=(%.0f,%.0f) Max=(%.0f,%.0f))."),
		Cells.Num(), EmptyPolyCount, Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y);
}

void AVoxelSplineNetworkGenerator::AssignBiomes()
{
	if (BiomeRows.IsEmpty()) return;
	FRandomStream Rand(Seed+1);
	TArray<bool> Assigned; Assigned.Init(false, Cells.Num());
	TQueue<int32> Q;
	const int32 Start = Rand.RandRange(0, Cells.Num()-1);
	Cells[Start].BiomeRowName = BiomeRowNames[Rand.RandRange(0, BiomeRowNames.Num()-1)];
	Assigned[Start] = true; Q.Enqueue(Start);
	while (!Q.IsEmpty())
	{
		int32 Cur; Q.Dequeue(Cur);
		for (int32 N : Cells[Cur].NeighborIndices)
		{
			if (Assigned[N]) continue; Assigned[N]=true; Q.Enqueue(N);
			TSet<int32> VS; bool bHas=false;
			for (int32 NN : Cells[N].NeighborIndices)
			{
				if (!Assigned[NN]) continue; bHas=true;
				const int32 Bi = BiomeRowNames.IndexOfByKey(Cells[NN].BiomeRowName);
				if (!BiomeRows.IsValidIndex(Bi)) continue;
				const FVoxelBiomeRow& NB = BiomeRows[Bi];
				if (NB.CanBorderBiomes.IsEmpty()) for (int32 B2=0;B2<BiomeRowNames.Num();++B2) VS.Add(B2);
				else for (int32 B2=0;B2<BiomeRowNames.Num();++B2)
					if (NB.CanBorderBiomes.Contains(BiomeRowNames[B2])) VS.Add(B2);
			}
			TArray<int32> VA = VS.Array();
			Cells[N].BiomeRowName = BiomeRowNames[(!bHas||VA.IsEmpty()) ? Rand.RandRange(0,BiomeRowNames.Num()-1) : VA[Rand.RandRange(0,VA.Num()-1)]];
		}
	}
	for (int32 i = 0; i < Cells.Num(); ++i)
		if (!Assigned[i]) Cells[i].BiomeRowName = BiomeRowNames[Rand.RandRange(0, BiomeRowNames.Num()-1)];
}

void AVoxelSplineNetworkGenerator::TraceTerrainHeights()
{
	int32 HitCount = 0, MissCount = 0, OnTerrainCount = 0;
	float MinHitZ = TNumericLimits<float>::Max(), MaxHitZ = -TNumericLimits<float>::Max();

	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [TraceTerrainHeights] Tracing %d cells. TraceStartZ=%.0f TraceEndZ=%.0f CoastHeightThreshold=%.0f MinSpawnHeight=%.0f"),
		Cells.Num(), TraceStartZ, TraceEndZ, CoastHeightThreshold, MinSpawnHeight);

	for (FVoxelBiomeCell& C : Cells)
	{
		FVector Hit;
		if (TraceGroundAt(C.WorldPosition.X, C.WorldPosition.Y, Hit))
		{
			++HitCount;
			MinHitZ = FMath::Min(MinHitZ, Hit.Z);
			MaxHitZ = FMath::Max(MaxHitZ, Hit.Z);
			C.WorldPosition.Z = Hit.Z;
			C.bOnTerrain = (Hit.Z > CoastHeightThreshold) && (Hit.Z >= MinSpawnHeight);
			if (C.bOnTerrain) ++OnTerrainCount;
		}
		else
		{
			++MissCount;
		}
	}

	if (HitCount > 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelSplineNetworkGenerator: [TraceTerrainHeights] Hits=%d Misses=%d OnTerrain=%d. Hit Z range: [%.0f, %.0f]"),
			HitCount, MissCount, OnTerrainCount, MinHitZ, MaxHitZ);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("VoxelSplineNetworkGenerator: [TraceTerrainHeights] ALL %d traces MISSED — every single line trace from Z=%.0f to Z=%.0f hit nothing on ECC_WorldStatic. "
			     "This means either: (1) cell XY positions don't overlap the voxel terrain at all, (2) the terrain's collision hasn't finished cooking/generating yet when this ran "
			     "(increase GenerationDelay), (3) the voxel world's collision response doesn't block ECC_WorldStatic, or (4) TraceStartZ/TraceEndZ don't bracket the terrain's actual Z range."),
			Cells.Num(), TraceStartZ, TraceEndZ);
	}
	if (HitCount > 0 && OnTerrainCount == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("VoxelSplineNetworkGenerator: [TraceTerrainHeights] Traces are hitting terrain (Z range [%.0f, %.0f]) but 0 cells qualify as bOnTerrain. "
			     "Check CoastHeightThreshold (%.0f) and MinSpawnHeight (%.0f) against that Z range — they're likely set above your terrain's actual height."),
			MinHitZ, MaxHitZ, CoastHeightThreshold, MinSpawnHeight);
	}
}

void AVoxelSplineNetworkGenerator::ComputeIslandIndices()
{
	for (FVoxelBiomeCell& C : Cells) C.IslandIndex = -1;
	int32 Next = 0;
	for (int32 Start = 0; Start < Cells.Num(); ++Start)
	{
		if (!Cells[Start].bOnTerrain || Cells[Start].IslandIndex >= 0) continue;
		TQueue<int32> Q; Cells[Start].IslandIndex = Next; Q.Enqueue(Start);
		while (!Q.IsEmpty())
		{
			int32 Cur; Q.Dequeue(Cur);
			for (int32 Nb : Cells[Cur].NeighborIndices)
			{
				if (!Cells.IsValidIndex(Nb)||!Cells[Nb].bOnTerrain||Cells[Nb].IslandIndex>=0) continue;
				Cells[Nb].IslandIndex = Next; Q.Enqueue(Nb);
			}
		}
		++Next;
	}
	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: Found %d island(s)."), Next);
}

void AVoxelSplineNetworkGenerator::AssignLocations()
{
	ForcePlacedBiomeCells.Empty();
	FRandomStream Rand(Seed + 2);
	TArray<FVector2D> AllPlaced;

	for (int32 Li = 0; Li < LocationRows.Num(); ++Li)
	{
		const FVoxelLocationRow& Loc    = LocationRows[Li];
		const FName&             LocKey = LocationRowNames[Li];

		TArray<int32> Candidates;
		for (int32 Ci = 0; Ci < Cells.Num(); ++Ci)
		{
			const FVoxelBiomeCell& C = Cells[Ci];
			if (!C.bOnTerrain || C.LocationRowName != NAME_None) continue;
			if (Loc.AllowedBiomes.Num() > 0 && !Loc.AllowedBiomes.Contains(C.BiomeRowName)) continue;
			Candidates.Add(Ci);
		}
		for (int32 i = Candidates.Num()-1; i > 0; --i) Candidates.Swap(i, Rand.RandRange(0, i));

		const float Spacings[3] = { Loc.MinSpacingFromOthers, Loc.MinSpacingFromOthers * 0.5f, 0.f };
		bool bPlaced = false;

		for (int32 Pass = 0; Pass < 3 && !bPlaced; ++Pass)
		{
			const float SSq = Spacings[Pass] * Spacings[Pass];
			for (int32 Ci : Candidates)
			{
				const FVector2D P(Cells[Ci].WorldPosition.X, Cells[Ci].WorldPosition.Y);
				bool bClose = false;
				if (SSq > 0.f)
					for (const FVector2D& E : AllPlaced)
						if (FVector2D::DistSquared(P,E) < SSq) { bClose=true; break; }
				if (!bClose) { Cells[Ci].LocationRowName=LocKey; AllPlaced.Add(P); bPlaced=true; break; }
			}
		}
		if (bPlaced) continue;

		if (!bForcePlaceLocations)
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSplineNetworkGenerator: Could not place '%s'."), *LocKey.ToString());
			continue;
		}

		TArray<int32> Fallback;
		for (int32 Ci = 0; Ci < Cells.Num(); ++Ci)
			if (Cells[Ci].bOnTerrain && Cells[Ci].LocationRowName == NAME_None)
				Fallback.Add(Ci);
		if (Fallback.IsEmpty())
			for (int32 Ci = 0; Ci < Cells.Num(); ++Ci)
				if (Cells[Ci].bOnTerrain) Fallback.Add(Ci);
		if (Fallback.IsEmpty())
		{
			UE_LOG(LogTemp, Error,
				TEXT("VoxelSplineNetworkGenerator: Force-place '%s' failed — no terrain cells!"), *LocKey.ToString());
			continue;
		}

		int32 BestCi = Fallback[0]; float BestMinDistSq = -1.f;
		for (int32 Ci : Fallback)
		{
			const FVector2D P(Cells[Ci].WorldPosition.X, Cells[Ci].WorldPosition.Y);
			float MinDistSq = FLT_MAX;
			if (AllPlaced.IsEmpty()) { BestCi=Ci; break; }
			for (const FVector2D& E : AllPlaced) MinDistSq=FMath::Min(MinDistSq, FVector2D::DistSquared(P,E));
			if (MinDistSq > BestMinDistSq) { BestMinDistSq=MinDistSq; BestCi=Ci; }
		}

		const bool bBiomeMismatch = Loc.AllowedBiomes.Num()>0 && !Loc.AllowedBiomes.Contains(Cells[BestCi].BiomeRowName);
		Cells[BestCi].LocationRowName = LocKey;
		AllPlaced.Add(FVector2D(Cells[BestCi].WorldPosition.X, Cells[BestCi].WorldPosition.Y));
		if (bBiomeMismatch && bForcePlaceBiomeCircle) ForcePlacedBiomeCells.Add(BestCi);
		UE_LOG(LogTemp, Warning,
			TEXT("VoxelSplineNetworkGenerator: Force-placed '%s' at cell %d (biome mismatch=%s)."),
			*LocKey.ToString(), BestCi, bBiomeMismatch ? TEXT("yes") : TEXT("no"));
	}
}

FVector2D AVoxelSplineNetworkGenerator::FindShorelinePoint(int32 CoastalCellIdx) const
{
	if (!Cells.IsValidIndex(CoastalCellIdx))
		return FVector2D::ZeroVector;

	const FVoxelBiomeCell& Cell = Cells[CoastalCellIdx];
	const FVector2D CellPos(Cell.WorldPosition.X, Cell.WorldPosition.Y);

	int32 WaterNeighbor = -1;
	float BestDistSq    = FLT_MAX;
	for (int32 Nb : Cell.NeighborIndices)
	{
		if (!Cells.IsValidIndex(Nb) || Cells[Nb].bOnTerrain) continue;
		const FVector2D NbPos(Cells[Nb].WorldPosition.X, Cells[Nb].WorldPosition.Y);
		const float DSq = FVector2D::DistSquared(CellPos, NbPos);
		if (DSq < BestDistSq) { BestDistSq = DSq; WaterNeighbor = Nb; }
	}

	if (WaterNeighbor < 0)
		return CellPos;

	const FVector2D WaterPos(Cells[WaterNeighbor].WorldPosition.X, Cells[WaterNeighbor].WorldPosition.Y);

	FVector2D Lo = CellPos;
	FVector2D Hi = WaterPos;
	for (int32 Iter = 0; Iter < 12; ++Iter)
	{
		const FVector2D Mid = (Lo + Hi) * 0.5f;
		FVector Hit;
		const float MidZ = TraceGroundAt(Mid.X, Mid.Y, Hit) ? Hit.Z : CoastHeightThreshold - 1.f;
		if (MidZ > CoastHeightThreshold) Lo = Mid;
		else                             Hi = Mid;
	}
	return (Lo + Hi) * 0.5f;
}

void AVoxelSplineNetworkGenerator::GatherDockCells()
{
	IslandDockCellMap.Empty();
	GenericDockCellSet.Empty();
	DockShorelinePointMap.Empty();

	TSet<int32> AllIslands;
	for (const FVoxelBiomeCell& C : Cells)
		if (C.bOnTerrain && C.IslandIndex >= 0)
			AllIslands.Add(C.IslandIndex);

	for (int32 IslandIdx : AllIslands)
	{
		int32 NamedDock = -1;
		for (int32 Ci = 0; Ci < Cells.Num(); ++Ci)
		{
			if (Cells[Ci].IslandIndex != IslandIdx || !Cells[Ci].bOnTerrain) continue;
			if (Cells[Ci].LocationRowName == NAME_None) continue;
			const int32 Li = LocationRowNames.IndexOfByKey(Cells[Ci].LocationRowName);
			if (LocationRows.IsValidIndex(Li) && LocationRows[Li].bConnectsToSeaLane)
			{ NamedDock = Ci; break; }
		}

		if (NamedDock >= 0)
		{
			IslandDockCellMap.Add(IslandIdx, NamedDock);
			DockShorelinePointMap.Add(NamedDock, FindShorelinePoint(NamedDock));
			UE_LOG(LogTemp, Log,
				TEXT("VoxelSplineNetworkGenerator: Island %d → named dock '%s' at cell %d."),
				IslandIdx, *Cells[NamedDock].LocationRowName.ToString(), NamedDock);
			continue;
		}

		if (!bAllSeaLanesHaveDocks) continue;
		
		int32 BestCoastalCell = -1;
		float BestCoastalZ    = FLT_MAX;
		int32 FallbackCell    = -1;
		float FallbackDelta   = FLT_MAX;

		for (int32 Ci = 0; Ci < Cells.Num(); ++Ci)
		{
			if (Cells[Ci].IslandIndex != IslandIdx || !Cells[Ci].bOnTerrain) continue;

			bool bTouchesWater = false;
			for (int32 Nb : Cells[Ci].NeighborIndices)
				if (!Cells.IsValidIndex(Nb) || !Cells[Nb].bOnTerrain) { bTouchesWater=true; break; }

			if (bTouchesWater)
			{
				if (Cells[Ci].WorldPosition.Z < BestCoastalZ)
				{ BestCoastalZ=Cells[Ci].WorldPosition.Z; BestCoastalCell=Ci; }
			}
			else
			{
				const float Delta = FMath::Abs(Cells[Ci].WorldPosition.Z - CoastHeightThreshold);
				if (Delta < FallbackDelta) { FallbackDelta=Delta; FallbackCell=Ci; }
			}
		}

		int32 BestCell = BestCoastalCell;
		if (BestCell < 0)
		{
			BestCell = FallbackCell;
			if (BestCell >= 0)
				UE_LOG(LogTemp, Warning,
					TEXT("VoxelSplineNetworkGenerator: Island %d — no coastal cell; "
					     "height-based fallback (cell %d, deltaZ=%.0f)."),
					IslandIdx, BestCell, FallbackDelta);
		}
		if (BestCell < 0) continue;

		IslandDockCellMap.Add(IslandIdx, BestCell);
		GenericDockCellSet.Add(BestCell);
		DockShorelinePointMap.Add(BestCell, FindShorelinePoint(BestCell));

		UE_LOG(LogTemp, Log,
			TEXT("VoxelSplineNetworkGenerator: Island %d → generic dock at cell %d (Z=%.0f, coastal=%s)."),
			IslandIdx, BestCell, Cells[BestCell].WorldPosition.Z,
			(BestCell==BestCoastalCell) ? TEXT("yes") : TEXT("no (fallback)"));
	}

	UE_LOG(LogTemp, Log,
		TEXT("VoxelSplineNetworkGenerator: GatherDockCells — %d total dock(s), %d generic."),
		IslandDockCellMap.Num(), GenericDockCellSet.Num());
}

void AVoxelSplineNetworkGenerator::SampleHeightGrid()
{
	const int32 Res = FMath::Max(CoastSampleResolution, 2);
	CachedGridRes = Res; CachedHeightGrid.SetNum(Res*Res);
	const FBox2D Bounds = GetNetworkBounds2D();
	const float SX = (Bounds.Max.X-Bounds.Min.X)/(Res-1), SY = (Bounds.Max.Y-Bounds.Min.Y)/(Res-1);
	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: Sampling %d x %d height grid..."), Res, Res);
	int32 GridHits = 0, GridMisses = 0;
	for (int32 Row=0; Row<Res; ++Row) { const float WY=Bounds.Min.Y+Row*SY;
		for (int32 Col=0; Col<Res; ++Col) { FVector Hit;
			if (TraceGroundAt(Bounds.Min.X+Col*SX, WY, Hit)) { CachedHeightGrid[Row*Res+Col] = Hit.Z; ++GridHits; }
			else { CachedHeightGrid[Row*Res+Col] = GRID_MISS; ++GridMisses; } } }
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [SampleHeightGrid] %d hits, %d misses out of %d samples (%.1f%% hit rate)."),
		GridHits, GridMisses, Res*Res, 100.f * GridHits / FMath::Max(1, Res*Res));
}

void AVoxelSplineNetworkGenerator::RasterizeBiomeGrid()
{
	const int32 Res = CachedGridRes;
	CachedBiomeGrid.Init(NAME_None, Res*Res);
	if (Cells.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelSplineNetworkGenerator: [RasterizeBiomeGrid] No cells — nothing to rasterize."));
		return;
	}
	const FBox2D Bounds = GetNetworkBounds2D();
	const float SX=(Bounds.Max.X-Bounds.Min.X)/(Res-1), SY=(Bounds.Max.Y-Bounds.Min.Y)/(Res-1);
	TArray<FVector2D> CP; CP.Reserve(Cells.Num());
	for (const FVoxelBiomeCell& C : Cells) CP.Add(FVector2D(C.WorldPosition.X, C.WorldPosition.Y));
	int32 LandCells = 0;
	for (int32 Row=0; Row<Res; ++Row) for (int32 Col=0; Col<Res; ++Col)
	{
		const float H = CachedHeightGrid[Row*Res+Col];
		if (H<=GRID_MISS*0.5f || H<CoastHeightThreshold) continue;
		const FVector2D WXY(Bounds.Min.X+Col*SX, Bounds.Min.Y+Row*SY);
		float BD=FLT_MAX; int32 BC=0;
		for (int32 Ci=0; Ci<CP.Num(); ++Ci) { const float D=FVector2D::DistSquared(WXY,CP[Ci]); if(D<BD){BD=D;BC=Ci;} }
		CachedBiomeGrid[Row*Res+Col] = Cells[BC].BiomeRowName;
		++LandCells;
	}
	UE_LOG(LogTemp, Warning,
		TEXT("VoxelSplineNetworkGenerator: [RasterizeBiomeGrid] %d / %d grid cells classified as land (H > CoastHeightThreshold=%.0f)."),
		LandCells, Res*Res, CoastHeightThreshold);
}

USplineComponent* AVoxelSplineNetworkGenerator::MakeSplineComponent(const FString& Name)
{
	USplineComponent* S = NewObject<USplineComponent>(this, *Name);
	S->SetupAttachment(GetRootComponent()); S->RegisterComponent();
	AddInstanceComponent(S); S->ClearSplinePoints(false);
	return S;
}

void AVoxelSplineNetworkGenerator::BuildCellSplines()
{
	if (CachedBiomeGrid.IsEmpty()) return;
	const int32 Res = CachedGridRes;
	const FBox2D Bounds = GetNetworkBounds2D();
	const float SX=(Bounds.Max.X-Bounds.Min.X)/(Res-1), SY=(Bounds.Max.Y-Bounds.Min.Y)/(Res-1);
	auto SH=[&](float WX,float WY)->float{
		const float FC=(WX-Bounds.Min.X)/SX, FR=(WY-Bounds.Min.Y)/SY;
		const int32 C0=FMath::Clamp(FMath::FloorToInt(FC),0,Res-1), R0=FMath::Clamp(FMath::FloorToInt(FR),0,Res-1);
		const int32 C1=FMath::Min(C0+1,Res-1), R1=FMath::Min(R0+1,Res-1);
		const float TX=FMath::Clamp(FC-C0,0.f,1.f), TY=FMath::Clamp(FR-R0,0.f,1.f);
		auto SafeH=[&](int32 R,int32 C)->float{ const float H=CachedHeightGrid[R*Res+C]; return(H>GRID_MISS*0.5f)?H:GRID_MISS; };
		const float H00=SafeH(R0,C0), H10=SafeH(R0,C1), H01=SafeH(R1,C0), H11=SafeH(R1,C1);
		float V=GRID_MISS; for(float H:{H00,H10,H01,H11}) if(H>GRID_MISS*0.5f){V=H;break;}
		const float T=(H00>GRID_MISS*0.5f&&H10>GRID_MISS*0.5f)?FMath::Lerp(H00,H10,TX):V;
		const float Bo=(H01>GRID_MISS*0.5f&&H11>GRID_MISS*0.5f)?FMath::Lerp(H01,H11,TX):V;
		return(T>GRID_MISS*0.5f&&Bo>GRID_MISS*0.5f)?FMath::Lerp(T,Bo,TY):V; };
	TSet<FName> PB; for(const FName& B:CachedBiomeGrid) if(B!=NAME_None) PB.Add(B);
	for (const FName& BN : PB)
	{
		const int32 Bi = BiomeRowNames.IndexOfByKey(BN); if (!BiomeRows.IsValidIndex(Bi)) continue;
		auto IF=[&](int32 R,int32 C)->bool{ return CachedBiomeGrid[R*Res+C]==BN; };
		auto VF=[&](int32 R,int32 C)->float{ return CachedBiomeGrid[R*Res+C]==BN?1.f:0.f; };
		const TArray<TArray<FVector2D>> Loops =
			FVoxelDelaunayVoronoiUtils::ExtractMarchingSquaresLoops(Res, Bounds, IF, VF, 0.5f, CoastMinLoopSegments);
		const FString SN=SanitiseName(BiomeRows[Bi].BiomeName); int32 LC=1;
		for (const TArray<FVector2D>& Loop : Loops)
		{
			USplineComponent* S = MakeSplineComponent(FString::Printf(TEXT("Biome_%s_%02d"),*SN,LC++));
			for (const FVector2D& P : Loop) { FVector Hit; float Z;
				if (TraceGroundAt(P.X,P.Y,Hit)) Z=Hit.Z+RoadZOffset;
				else { const float CH=SH(P.X,P.Y); Z=(CH>GRID_MISS*0.5f)?CH+RoadZOffset:RoadZOffset; }
				S->AddSplinePoint(FVector(P.X,P.Y,Z), ESplineCoordinateSpace::World, false); }
			S->SetClosedLoop(true,false); S->UpdateSpline();
			S->ComponentTags.Add(BiomeRows[Bi].PCGTag); BiomeSplines.Add(S);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: %d biome splines."), BiomeSplines.Num());
}

void AVoxelSplineNetworkGenerator::BuildCentreSplines()
{
	const int32 Segs = 12;
	TMap<FString,int32> NC;

	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		const FVoxelBiomeCell& Cell = Cells[i];
		if (!Cell.bOnTerrain) continue;

		const int32 Bi = BiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
		if (!BiomeRows.IsValidIndex(Bi)) continue;

		const bool  bIsLoc = Cell.LocationRowName != NAME_None;
		const int32 Li     = bIsLoc ? LocationRowNames.IndexOfByKey(Cell.LocationRowName) : INDEX_NONE;
		const float Radius = (bIsLoc && LocationRows.IsValidIndex(Li)) ? LocationRows[Li].CentreRadius : 10000.f;

		FString SplineName;
		if (bIsLoc && LocationRows.IsValidIndex(Li))
			SplineName = FString::Printf(TEXT("Centre_%s"), *SanitiseName(LocationRows[Li].DisplayName));
		else
		{
			const FString S2 = SanitiseName(BiomeRows[Bi].BiomeName);
			SplineName = FString::Printf(TEXT("Centre_%s_%02d"), *S2, NC.FindOrAdd(S2)++ + 1);
		}

		USplineComponent* S = MakeSplineComponent(SplineName);
		for (int32 Seg = 0; Seg < Segs; ++Seg)
		{
			const float A  = ((float)Seg / Segs) * 2.f * PI;
			const float PX = Cell.WorldPosition.X + FMath::Cos(A) * Radius;
			const float PY = Cell.WorldPosition.Y + FMath::Sin(A) * Radius;
			FVector Hit;
			const float Z = TraceGroundAt(PX,PY,Hit) ? Hit.Z+RoadZOffset : Cell.WorldPosition.Z+RoadZOffset;
			S->AddSplinePoint(FVector(PX,PY,Z), ESplineCoordinateSpace::World, false);
		}
		S->SetClosedLoop(true,false); S->UpdateSpline();
		S->ComponentTags.Add(BiomeRows[Bi].PCGTag);

		if (bIsLoc && LocationRows.IsValidIndex(Li))
		{
			const FVoxelLocationRow& LR = LocationRows[Li];
			S->ComponentTags.Add(LR.PCGTag); S->ComponentTags.Add(LR.CategoryTag);
			if (LR.bConnectsToSeaLane) S->ComponentTags.Add(TEXT("PCG_Dock"));
		}
		else S->ComponentTags.Add(TEXT("PCG_CellCentre"));

		CentreSplines.Add(S);

		if (bIsLoc && bForcePlaceBiomeCircle && ForcePlacedBiomeCells.Contains(i)
		    && LocationRows.IsValidIndex(Li))
		{
			const FVoxelLocationRow& LR = LocationRows[Li];
			const float BiomeRadius     = Radius * 2.f;

			USplineComponent* BS = MakeSplineComponent(
				FString::Printf(TEXT("ForcedBiome_%s"), *SanitiseName(LR.DisplayName)));
			for (int32 Seg = 0; Seg < Segs; ++Seg)
			{
				const float A  = ((float)Seg / Segs) * 2.f * PI;
				const float PX = Cell.WorldPosition.X + FMath::Cos(A) * BiomeRadius;
				const float PY = Cell.WorldPosition.Y + FMath::Sin(A) * BiomeRadius;
				FVector Hit;
				const float Z = TraceGroundAt(PX,PY,Hit) ? Hit.Z+RoadZOffset : Cell.WorldPosition.Z+RoadZOffset;
				BS->AddSplinePoint(FVector(PX,PY,Z), ESplineCoordinateSpace::World, false);
			}
			BS->SetClosedLoop(true,false); BS->UpdateSpline();
			if (!LR.AllowedBiomes.IsEmpty())
			{
				const int32 Bi2 = BiomeRowNames.IndexOfByKey(LR.AllowedBiomes[0]);
				if (BiomeRows.IsValidIndex(Bi2)) BS->ComponentTags.Add(BiomeRows[Bi2].PCGTag);
			}
			BS->ComponentTags.Add(TEXT("PCG_ForcedBiome"));
			BS->ComponentTags.Add(LR.PCGTag);
			CentreSplines.Add(BS);
		}
	}
}

void AVoxelSplineNetworkGenerator::BuildDockSplines()
{
}

void AVoxelSplineNetworkGenerator::BuildRoadSplines()
{
	TMap<int32, TArray<int32>> IslandNodes;

	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		if (Cells[i].LocationRowName == NAME_None || Cells[i].IslandIndex < 0) continue;
		const int32 Li = LocationRowNames.IndexOfByKey(Cells[i].LocationRowName);
		if (!LocationRows.IsValidIndex(Li) || !LocationRows[Li].bConnectsToRoadNetwork) continue;
		IslandNodes.FindOrAdd(Cells[i].IslandIndex).Add(i);
	}
	for (auto& KV : IslandDockCellMap)
		IslandNodes.FindOrAdd(KV.Key).AddUnique(KV.Value);

	for (auto& KV : IslandNodes)
	{
		const TArray<int32>& Nodes = KV.Value;
		if (Nodes.Num() < 2) continue;

		TSet<int32> InMST; InMST.Add(Nodes[0]);
		while (InMST.Num() < Nodes.Num())
		{
			float BD=FLT_MAX; int32 BF=-1, BT=-1;
			for (int32 F : InMST) for (int32 T : Nodes)
			{
				if (InMST.Contains(T)) continue;
				const float D = FVector2D::Distance(
					FVector2D(Cells[F].WorldPosition.X,Cells[F].WorldPosition.Y),
					FVector2D(Cells[T].WorldPosition.X,Cells[T].WorldPosition.Y));
				if (D<BD){BD=D;BF=F;BT=T;}
			}
			if (BF < 0) break;
			InMST.Add(BT);

			TArray<int32> Path = BFSPath(BF, BT);

			auto NodeName = [&](int32 Ci) -> FString
			{
				if (GenericDockCellSet.Contains(Ci)) return FString::Printf(TEXT("GenericDock_%d"), Ci);
				const int32 Li = LocationRowNames.IndexOfByKey(Cells[Ci].LocationRowName);
				return LocationRows.IsValidIndex(Li)
					? SanitiseName(LocationRows[Li].DisplayName)
					: FString::Printf(TEXT("Cell%d"), Ci);
			};

			if (Path.Num() > 1 && Path.Num() <= MaxRoadPathHops + 1)
				CreateRoadSpline(Path, NodeName(BF), NodeName(BT));
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("VoxelSplineNetworkGenerator: No land path '%s'(%d)<->'%s'(%d) — water crossing."),
					*NodeName(BF), BF, *NodeName(BT), BT);
				CreateWaterCrossingConnection(BF, BT);
			}
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
		if (Cur == End)
		{
			TArray<int32> P;
			for (int32 N=End; N!=-1; N=CF[N]) P.Insert(N,0);
			return P;
		}
		for (int32 N : Cells[Cur].NeighborIndices)
		{
			if (CF.Contains(N)||!Cells.IsValidIndex(N)) continue;
			if (!Cells[N].bOnTerrain) continue;
			if (Cells[N].WorldPosition.Z <= CoastHeightThreshold) continue;
			if (Cells[N].WorldPosition.Z <  MinRoadHeight) continue;
			CF.Add(N, Cur); Q.Enqueue(N);
		}
	}
	return {};
}

int32 AVoxelSplineNetworkGenerator::FindNearestReachableCoastalCell(int32 FromCellIdx) const
{
	if (!Cells.IsValidIndex(FromCellIdx)) return -1;

	auto IsCoastal = [&](int32 Ci) -> bool
	{
		if (!Cells.IsValidIndex(Ci) || !Cells[Ci].bOnTerrain) return false;
		for (int32 Nb : Cells[Ci].NeighborIndices)
			if (!Cells.IsValidIndex(Nb) || !Cells[Nb].bOnTerrain) return true;
		return false;
	};

	if (IsCoastal(FromCellIdx)) return FromCellIdx;

	TMap<int32,int32> Visited; TQueue<int32> Q;
	Visited.Add(FromCellIdx,-1); Q.Enqueue(FromCellIdx);
	while (!Q.IsEmpty())
	{
		int32 Cur; Q.Dequeue(Cur);
		if (Cur != FromCellIdx && IsCoastal(Cur)) return Cur;
		for (int32 N : Cells[Cur].NeighborIndices)
		{
			if (Visited.Contains(N)||!Cells.IsValidIndex(N)) continue;
			if (!Cells[N].bOnTerrain || Cells[N].WorldPosition.Z <= CoastHeightThreshold) continue;
			Visited.Add(N, Cur); Q.Enqueue(N);
		}
	}
	return -1;
}

void AVoxelSplineNetworkGenerator::StampEmergencyDockAtCell(int32 DockCi)
{
	if (GenericDockCellSet.Contains(DockCi)) return;

	const int32 Li = LocationRowNames.IndexOfByKey(Cells[DockCi].LocationRowName);
	if (LocationRows.IsValidIndex(Li) && LocationRows[Li].bConnectsToSeaLane) return;

	GenericDockCellSet.Add(DockCi);
	const FVector2D Shoreline = FindShorelinePoint(DockCi);
	DockShorelinePointMap.Add(DockCi, Shoreline);
}

void AVoxelSplineNetworkGenerator::CreateWaterCrossingConnection(int32 FromCell, int32 ToCell)
{
	const int32 CoastalF = FindNearestReachableCoastalCell(FromCell);
	const int32 CoastalT = FindNearestReachableCoastalCell(ToCell);

	if (CoastalF < 0 || CoastalT < 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("VoxelSplineNetworkGenerator: CreateWaterCrossingConnection — "
			     "no coastal cell for %d <-> %d. Skipping."), FromCell, ToCell);
		return;
	}

	if (CoastalF == CoastalT)
	{
		TArray<int32> PathF = BFSPath(FromCell, CoastalF);
		TArray<int32> PathT = BFSPath(ToCell,   CoastalT);
		if (PathF.Num() > 1) CreateRoadSpline(PathF,
			FString::Printf(TEXT("Cell%d"),FromCell), FString::Printf(TEXT("Coast%d"),CoastalF));
		if (PathT.Num() > 1 && ToCell != CoastalT) CreateRoadSpline(PathT,
			FString::Printf(TEXT("Cell%d"),ToCell), FString::Printf(TEXT("Coast%d"),CoastalT));
		return;
	}
	
	if (FromCell != CoastalF)
	{
		TArray<int32> PathF = BFSPath(FromCell, CoastalF);
		if (PathF.Num() > 1) CreateRoadSpline(PathF,
			FString::Printf(TEXT("Cell%d"),FromCell),
			FString::Printf(TEXT("EmergencyDock_%d"),CoastalF));
	}
	if (ToCell != CoastalT)
	{
		TArray<int32> PathT = BFSPath(ToCell, CoastalT);
		if (PathT.Num() > 1) CreateRoadSpline(PathT,
			FString::Printf(TEXT("Cell%d"),ToCell),
			FString::Printf(TEXT("EmergencyDock_%d"),CoastalT));
	}
	
	StampEmergencyDockAtCell(CoastalF);
	StampEmergencyDockAtCell(CoastalT);

	const FVector2D PosF = DockShorelinePointMap.Contains(CoastalF)
		? DockShorelinePointMap[CoastalF]
		: FVector2D(Cells[CoastalF].WorldPosition.X, Cells[CoastalF].WorldPosition.Y);
	const FVector2D PosT = DockShorelinePointMap.Contains(CoastalT)
		? DockShorelinePointMap[CoastalT]
		: FVector2D(Cells[CoastalT].WorldPosition.X, Cells[CoastalT].WorldPosition.Y);

	const FVector2D Dir      = (PosT - PosF).GetSafeNormal();
	const FVector2D StartPos = PosF + Dir *  GenericDockRadius;
	const FVector2D EndPos   = PosT + Dir * -GenericDockRadius;
	const float     WaterZ   = SeaLaneZHeight + SeaLaneZOffset;
	const int32     Segs     = 24;
	
	USplineComponent* SL = MakeSplineComponent(
		FString::Printf(TEXT("SeaLane_Emergency_%d_to_%d_%02d"),
			CoastalF, CoastalT, SeaLaneSplines.Num()+1));

	SL->AddSplinePoint(FVector(StartPos.X, StartPos.Y, WaterZ), ESplineCoordinateSpace::World, false);
	for (int32 Si=1; Si<SeaLaneIntermediateSamples; ++Si)
	{
		const float T = (float)Si / SeaLaneIntermediateSamples;
		const FVector2D P = FMath::Lerp(StartPos, EndPos, T);
		SL->AddSplinePoint(FVector(P.X, P.Y, WaterZ), ESplineCoordinateSpace::World, false);
	}
	SL->AddSplinePoint(FVector(EndPos.X, EndPos.Y, WaterZ), ESplineCoordinateSpace::World, false);
	SL->UpdateSpline();
	SL->ComponentTags.Add(TEXT("PCG_SeaLane"));
	SeaLaneSplines.Add(SL);
	
	auto MakeDockCircle = [&](int32 DockCi, const FVector2D& Centre, const FString& Tag)
	{
		const int32 LiD = LocationRowNames.IndexOfByKey(Cells[DockCi].LocationRowName);
		const float R   = LocationRows.IsValidIndex(LiD) ? LocationRows[LiD].CentreRadius : GenericDockRadius;
		USplineComponent* D = MakeSplineComponent(Tag);
		for (int32 Seg=0; Seg<Segs; ++Seg)
		{
			const float A  = ((float)Seg / Segs) * 2.f * PI;
			D->AddSplinePoint(FVector(Centre.X + FMath::Cos(A)*R, Centre.Y + FMath::Sin(A)*R, WaterZ),
				ESplineCoordinateSpace::World, false);
		}
		D->SetClosedLoop(true,false); D->UpdateSpline();
		D->ComponentTags.Add(TEXT("PCG_Dock"));
		D->ComponentTags.Add(TEXT("PCG_GenericDock"));
		DockSplines.Add(D);
	};

	MakeDockCircle(CoastalF, PosF, FString::Printf(TEXT("Dock_Emergency_%d"), CoastalF));
	MakeDockCircle(CoastalT, PosT, FString::Printf(TEXT("Dock_Emergency_%d"), CoastalT));

	UE_LOG(LogTemp, Log,
		TEXT("VoxelSplineNetworkGenerator: Emergency crossing: %d→coast%d~~[sea]~~coast%d←%d."),
		FromCell, CoastalF, CoastalT, ToCell);
}

void AVoxelSplineNetworkGenerator::CreateRoadSpline(
	const TArray<int32>& Path, const FString& NameA, const FString& NameB)
{
	if (Path.Num() < 2) return;
	USplineComponent* S = MakeSplineComponent(
		FString::Printf(TEXT("Road_%s_to_%s_%02d"), *NameA, *NameB, RoadSplines.Num()+1));
	
	auto IsOverLand = [&](const FVector2D& P) -> bool
	{
		FVector Hit;
		return TraceGroundAt(P.X, P.Y, Hit) && Hit.Z > CoastHeightThreshold;
	};

	auto GetEndpointBase = [&](int32 Ci) -> FVector2D
	{
		if (IsRegisteredDockCell(Ci) && DockShorelinePointMap.Contains(Ci))
			return DockShorelinePointMap[Ci];
		return FVector2D(Cells[Ci].WorldPosition.X, Cells[Ci].WorldPosition.Y);
	};

	const FVector2D PosStart = GetEndpointBase(Path[0]);
	const FVector2D PosEnd   = GetEndpointBase(Path.Last());
	const FVector2D DirFromStart = (FVector2D(Cells[Path[1]].WorldPosition) - PosStart).GetSafeNormal();
	const FVector2D DirFromEnd   = (FVector2D(Cells[Path[Path.Num()-2]].WorldPosition) - PosEnd).GetSafeNormal();
	const FVector2D OffsetStart  = PosStart + DirFromStart * DockRadiusForCell(Path[0]);
	const FVector2D OffsetEnd    = PosEnd   + DirFromEnd   * DockRadiusForCell(Path.Last());

	FRandomStream JR(Seed + RoadSplines.Num() * 7919 + Path[0] * 31);

	TArray<FVector2D> Waypoints;
	Waypoints.Add(OffsetStart);
	const int32 SubDiv = FMath::Max(RoadPathSubdivisions, 0);
	for (int32 Pi=0; Pi<Path.Num()-1; ++Pi)
	{
		const FVector2D WpA = (Pi==0)            ? OffsetStart : FVector2D(Cells[Path[Pi]].WorldPosition);
		const FVector2D WpB = (Pi==Path.Num()-2) ? OffsetEnd   : FVector2D(Cells[Path[Pi+1]].WorldPosition);
		const FVector2D Fwd = (WpB-WpA).GetSafeNormal();
		const FVector2D Perp(-Fwd.Y, Fwd.X);
		for (int32 Si=1; Si<=SubDiv; ++Si)
		{
			const float T = (float)Si / (SubDiv+1);
			FVector2D Pos = FMath::Lerp(WpA, WpB, T);
			if (RoadLateralJitter > 0.f)
			{
				const float JitterScale = FMath::Sin(PI * T) * RoadLateralJitter;
				const FVector2D Candidate = Pos + Perp * JR.FRandRange(-JitterScale, JitterScale);
				if (IsOverLand(Candidate))
					Pos = Candidate;
				else
					JR.FRandRange(-JitterScale, JitterScale);
			}
			Waypoints.Add(Pos);
		}
		if (Pi < Path.Num()-2) Waypoints.Add(WpB);
	}
	Waypoints.Add(OffsetEnd);

	TArray<float> WaypointZ; WaypointZ.Reserve(Waypoints.Num());
	for (const FVector2D& WP : Waypoints) WaypointZ.Add(SafeRoadTerrainZ(WP.X, WP.Y, MinRoadHeight));

	for (int32 Wi=0; Wi<Waypoints.Num(); ++Wi)
	{
		S->AddSplinePoint(FVector(Waypoints[Wi].X, Waypoints[Wi].Y, WaypointZ[Wi]+RoadAboveTerrainLift),
			ESplineCoordinateSpace::World, false);
		if (Wi+1 < Waypoints.Num() && RoadIntermediateSamples > 0)
		{
			const FVector2D& WpA=Waypoints[Wi]; const FVector2D& WpB=Waypoints[Wi+1];
			const FVector2D Dir=(WpB-WpA).GetSafeNormal(); const FVector2D Perp(-Dir.Y, Dir.X);
			for (int32 Si=1; Si<RoadIntermediateSamples; ++Si)
			{
				const float T = (float)Si / RoadIntermediateSamples;
				FVector2D L = FMath::Lerp(WpA, WpB, T);
				if (RoadLateralJitter > 0.f)
				{
					const float JitterScale = FMath::Sin(PI * T) * RoadLateralJitter * 0.25f;
					const FVector2D Candidate = L + Perp * JR.FRandRange(-JitterScale, JitterScale);
					if (IsOverLand(Candidate))
						L = Candidate;
					else
						JR.FRandRange(-JitterScale, JitterScale);
				}
				const float FZ = FMath::Lerp(WaypointZ[Wi], WaypointZ[Wi+1], T);
				S->AddSplinePoint(FVector(L.X, L.Y, SafeRoadTerrainZ(L.X, L.Y, FZ)+RoadAboveTerrainLift),
					ESplineCoordinateSpace::World, false);
			}
		}
	}
	S->UpdateSpline();
	S->ComponentTags.Add(TEXT("PCG_Road"));
	RoadSplines.Add(S);
}

void AVoxelSplineNetworkGenerator::BuildSeaLaneSplines()
{
	const float WaterZ = SeaLaneZHeight + SeaLaneZOffset;
	const int32 DockSegs = 24;

	TArray<int32> IslandIds;
	IslandDockCellMap.GetKeys(IslandIds);
	if (IslandIds.Num() < 2)
	{
		UE_LOG(LogTemp, Log,
			TEXT("VoxelSplineNetworkGenerator: %d island(s) with docks — no sea lanes."), IslandIds.Num());
		return;
	}

	auto Dist2D = [&](int32 A, int32 B) -> float
	{
		return FVector2D::Distance(
			FVector2D(Cells[A].WorldPosition.X, Cells[A].WorldPosition.Y),
			FVector2D(Cells[B].WorldPosition.X, Cells[B].WorldPosition.Y));
	};
	
	TSet<int32> InMST; TArray<TPair<int32,int32>> MSTEdges;
	InMST.Add(IslandIds[0]);
	while (InMST.Num() < IslandIds.Num())
	{
		float Best=FLT_MAX; int32 BF=-1, BT=-1;
		for (int32 From : InMST) for (int32 To : IslandIds)
		{
			if (InMST.Contains(To)) continue;
			const float D = Dist2D(IslandDockCellMap[From], IslandDockCellMap[To]);
			if (D<Best){Best=D;BF=From;BT=To;}
		}
		if (BF<0) break;
		InMST.Add(BT); MSTEdges.Add({BF,BT});
	}
	
	TSet<int32> DockCirclesBuilt;
	
	auto BuildDockCircle = [&](int32 DockCi, const FVector2D& Centre, const FString& SplineName)
	{
		const bool bGen = GenericDockCellSet.Contains(DockCi);
		const int32 LiD = LocationRowNames.IndexOfByKey(Cells.IsValidIndex(DockCi)
			? Cells[DockCi].LocationRowName : NAME_None);
		const float R = (!bGen && LocationRows.IsValidIndex(LiD))
			? LocationRows[LiD].CentreRadius
			: GenericDockRadius;

		USplineComponent* D = MakeSplineComponent(SplineName);
		for (int32 Seg=0; Seg<DockSegs; ++Seg)
		{
			const float A = ((float)Seg / DockSegs) * 2.f * PI;
			D->AddSplinePoint(
				FVector(Centre.X + FMath::Cos(A)*R, Centre.Y + FMath::Sin(A)*R, WaterZ),
				ESplineCoordinateSpace::World, false);
		}
		D->SetClosedLoop(true,false); D->UpdateSpline();
		D->ComponentTags.Add(TEXT("PCG_Dock"));
		if (bGen)
		{
			D->ComponentTags.Add(TEXT("PCG_GenericDock"));
		}
		else if (LocationRows.IsValidIndex(LiD))
		{
			D->ComponentTags.Add(LocationRows[LiD].PCGTag);
			D->ComponentTags.Add(LocationRows[LiD].CategoryTag);
		}
		DockSplines.Add(D);
		UE_LOG(LogTemp, Log,
			TEXT("VoxelSplineNetworkGenerator: Dock '%s' at (%.0f, %.0f) Z=%.0f r=%.0f."),
			*SplineName, Centre.X, Centre.Y, WaterZ, R);
	};

	for (const TPair<int32,int32>& IE : MSTEdges)
	{
		const int32 CiA = IslandDockCellMap[IE.Key];
		const int32 CiB = IslandDockCellMap[IE.Value];
		const bool bGenA = GenericDockCellSet.Contains(CiA);
		const bool bGenB = GenericDockCellSet.Contains(CiB);
		const int32 LiA  = LocationRowNames.IndexOfByKey(Cells[CiA].LocationRowName);
		const int32 LiB  = LocationRowNames.IndexOfByKey(Cells[CiB].LocationRowName);
		const FString NA = (!bGenA&&LocationRows.IsValidIndex(LiA))
			? SanitiseName(LocationRows[LiA].DisplayName) : FString::Printf(TEXT("GenericDock%d"),CiA);
		const FString NB = (!bGenB&&LocationRows.IsValidIndex(LiB))
			? SanitiseName(LocationRows[LiB].DisplayName) : FString::Printf(TEXT("GenericDock%d"),CiB);
		
		const FVector2D PosA = DockShorelinePointMap.Contains(CiA)
			? DockShorelinePointMap[CiA]
			: FVector2D(Cells[CiA].WorldPosition.X, Cells[CiA].WorldPosition.Y);
		const FVector2D PosB = DockShorelinePointMap.Contains(CiB)
			? DockShorelinePointMap[CiB]
			: FVector2D(Cells[CiB].WorldPosition.X, Cells[CiB].WorldPosition.Y);

		const FVector2D DirAtoB  = (PosB - PosA).GetSafeNormal();
		const FVector2D StartPos = PosA + DirAtoB *  DockRadiusForCell(CiA);
		const FVector2D EndPos   = PosB + DirAtoB * -DockRadiusForCell(CiB);
		
		USplineComponent* S = MakeSplineComponent(
			FString::Printf(TEXT("SeaLane_%s_to_%s_%02d"), *NA, *NB, SeaLaneSplines.Num()+1));

		S->AddSplinePoint(FVector(StartPos.X, StartPos.Y, WaterZ), ESplineCoordinateSpace::World, false);
		for (int32 Si=1; Si<SeaLaneIntermediateSamples; ++Si)
		{
			const float T = (float)Si / SeaLaneIntermediateSamples;
			const FVector2D P = FMath::Lerp(StartPos, EndPos, T);
			S->AddSplinePoint(FVector(P.X, P.Y, WaterZ), ESplineCoordinateSpace::World, false);
		}
		S->AddSplinePoint(FVector(EndPos.X, EndPos.Y, WaterZ), ESplineCoordinateSpace::World, false);

		S->UpdateSpline();
		S->ComponentTags.Add(TEXT("PCG_SeaLane"));
		if (!bGenA && LocationRows.IsValidIndex(LiA)) S->ComponentTags.Add(LocationRows[LiA].PCGTag);
		if (!bGenB && LocationRows.IsValidIndex(LiB)) S->ComponentTags.Add(LocationRows[LiB].PCGTag);
		SeaLaneSplines.Add(S);
		
		if (!DockCirclesBuilt.Contains(CiA))
		{
			DockCirclesBuilt.Add(CiA);
			BuildDockCircle(CiA, PosA,
				FString::Printf(TEXT("Dock_%s"), *NA));
		}
		if (!DockCirclesBuilt.Contains(CiB))
		{
			DockCirclesBuilt.Add(CiB);
			BuildDockCircle(CiB, PosB,
				FString::Printf(TEXT("Dock_%s"), *NB));
		}

		UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: Sea lane %s <-> %s."), *NA, *NB);
	}
	UE_LOG(LogTemp, Log,
		TEXT("VoxelSplineNetworkGenerator: %d sea lane(s), %d dock circle(s)."),
		SeaLaneSplines.Num(), DockSplines.Num());
}

void AVoxelSplineNetworkGenerator::BuildCoastlineSplines()
{
	if (CachedHeightGrid.IsEmpty()) return;
	const int32 Res = CachedGridRes;
	auto IF=[&](int32 R,int32 C)->bool{ const float H=CachedHeightGrid[R*Res+C]; return H>GRID_MISS*0.5f&&H>=CoastHeightThreshold; };
	auto VF=[&](int32 R,int32 C)->float{ const float H=CachedHeightGrid[R*Res+C]; return(H>GRID_MISS*0.5f)?H:GRID_MISS; };
	const TArray<TArray<FVector2D>> Loops =
		FVoxelDelaunayVoronoiUtils::ExtractMarchingSquaresLoops(Res, GetNetworkBounds2D(), IF, VF, CoastHeightThreshold, CoastMinLoopSegments);
	for (const TArray<FVector2D>& Loop : Loops)
	{
		USplineComponent* S = MakeSplineComponent(FString::Printf(TEXT("Coast_%02d"),CoastSplines.Num()+1));
		for (const FVector2D& P : Loop) { FVector Hit;
			const float Z = TraceGroundAt(P.X,P.Y,Hit) ? Hit.Z+CoastZOffset : CoastHeightThreshold+CoastZOffset;
			S->AddSplinePoint(FVector(P.X,P.Y,Z), ESplineCoordinateSpace::World, false); }
		S->SetClosedLoop(true,false); S->UpdateSpline();
		S->ComponentTags.Add(TEXT("PCG_Coast")); CoastSplines.Add(S);
	}
	UE_LOG(LogTemp, Log, TEXT("VoxelSplineNetworkGenerator: %d coastline loop(s)."), CoastSplines.Num());
}

void AVoxelSplineNetworkGenerator::RebuildFillMesh()
{
	if (!FillMeshComponent) return;
	FillMeshComponent->ClearAllMeshSections();
	if (!bShowCellFill) return;
	for (int32 Ci=0; Ci<Cells.Num(); ++Ci)
	{
		const FVoxelBiomeCell& Cell=Cells[Ci];
		if (!Cell.bOnTerrain||Cell.VoronoiPolygon2D.Num()<3) continue;
		const FColor Col=GetBiomeColor(Cell.BiomeRowName, CellFillBrightness);
		FVector CH; const float CZ=TraceGroundAt(Cell.WorldPosition.X,Cell.WorldPosition.Y,CH)?CH.Z+DebugZLift:Cell.WorldPosition.Z+DebugZLift;
		TArray<FVector> Vs; TArray<int32> Ts; TArray<FVector> Ns; TArray<FVector2D> UVs; TArray<FColor> VCs;
		Vs.Add(FVector(Cell.WorldPosition.X,Cell.WorldPosition.Y,CZ)); Ns.Add(FVector::UpVector); UVs.Add(FVector2D(.5f,.5f)); VCs.Add(Col);
		const int32 N=Cell.VoronoiPolygon2D.Num();
		for (int32 Vi=0; Vi<N; ++Vi) { const FVector2D& V=Cell.VoronoiPolygon2D[Vi];
			FVector Hit; const float Z=TraceGroundAt(V.X,V.Y,Hit)?Hit.Z+DebugZLift:CZ;
			Vs.Add(FVector(V.X,V.Y,Z)); Ns.Add(FVector::UpVector); UVs.Add(FVector2D((float)Vi/N,1.f)); VCs.Add(Col); }
		for (int32 Vi=0; Vi<N; ++Vi) { const int32 A=1+Vi, B=1+(Vi+1)%N; Ts.Add(0);Ts.Add(A);Ts.Add(B);Ts.Add(0);Ts.Add(B);Ts.Add(A); }
		TArray<FProcMeshTangent> Tans;
		FillMeshComponent->CreateMeshSection(Ci,Vs,Ts,Ns,UVs,VCs,Tans,false);
		if (CellFillMaterial) FillMeshComponent->SetMaterial(Ci, CellFillMaterial);
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugEdges() const
{
	for (int32 i=0; i<Cells.Num(); ++i)
	{
		if (!Cells[i].bOnTerrain) continue;
		const FVector F = Cells[i].WorldPosition + FVector(0,0,DebugZLift);
		for (int32 N : Cells[i].NeighborIndices)
		{
			if (N<=i||!Cells.IsValidIndex(N)||!Cells[N].bOnTerrain) continue;
			DrawDebugLine(GetWorld(), F, Cells[N].WorldPosition+FVector(0,0,DebugZLift), EdgeColor, false, DebugDuration, 0, EdgeThickness);
		}
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugLocationMarkers() const
{
	for (const FVoxelBiomeCell& Cell : Cells)
	{
		if (!Cell.bOnTerrain||Cell.LocationRowName==NAME_None) continue;
		const int32 Li = LocationRowNames.IndexOfByKey(Cell.LocationRowName);
		if (!LocationRows.IsValidIndex(Li)) continue;
		DrawDebugSphere(GetWorld(),
			Cell.WorldPosition+FVector(0,0,DebugZLift),
			LocationRows[Li].CentreRadius, 12,
			LocationRows[Li].DebugColor.ToFColor(true), false, DebugDuration);
	}
	for (int32 DockCi : GenericDockCellSet)
	{
		if (!Cells.IsValidIndex(DockCi)||!Cells[DockCi].bOnTerrain) continue;
		const FVector DockPos = DockShorelinePointMap.Contains(DockCi)
			? FVector(DockShorelinePointMap[DockCi].X, DockShorelinePointMap[DockCi].Y,
			          Cells[DockCi].WorldPosition.Z+DebugZLift)
			: Cells[DockCi].WorldPosition+FVector(0,0,DebugZLift);
		DrawDebugSphere(GetWorld(), DockPos, GenericDockRadius, 12, FColor::White, false, DebugDuration);
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugDocks() const
{
	for (USplineComponent* Sp : DockSplines)
	{
		if (!Sp) continue;
		const int32 Steps = FMath::Max(Sp->GetNumberOfSplinePoints()*4, 32);
		const float Len   = Sp->GetSplineLength();
		for (int32 s=0; s<Steps; ++s)
		{
			const FVector P0 = Sp->GetLocationAtDistanceAlongSpline(((float)s/Steps)*Len,     ESplineCoordinateSpace::World);
			const FVector P1 = Sp->GetLocationAtDistanceAlongSpline(((float)(s+1)/Steps)*Len, ESplineCoordinateSpace::World);
			DrawDebugLine(GetWorld(), P0, P1, DockDebugColor, false, DebugDuration, 0, DockDebugThickness);
		}
	}
}

void AVoxelSplineNetworkGenerator::DrawDebugRoads() const
{
	for (USplineComponent* Sp : RoadSplines) { if (!Sp) continue;
		const int32 Steps=FMath::Max(Sp->GetNumberOfSplinePoints()*8,20); const float Len=Sp->GetSplineLength();
		for (int32 s=0; s<Steps; ++s) {
			DrawDebugLine(GetWorld(),
				Sp->GetLocationAtDistanceAlongSpline(((float)s/Steps)*Len,ESplineCoordinateSpace::World),
				Sp->GetLocationAtDistanceAlongSpline(((float)(s+1)/Steps)*Len,ESplineCoordinateSpace::World),
				RoadDebugColor,false,DebugDuration,0,RoadDebugThickness); } }
}

void AVoxelSplineNetworkGenerator::DrawDebugSeaLanes() const
{
	for (USplineComponent* Sp : SeaLaneSplines) { if (!Sp) continue;
		const int32 Steps=FMath::Max(Sp->GetNumberOfSplinePoints()*8,20); const float Len=Sp->GetSplineLength();
		for (int32 s=0; s<Steps; ++s) {
			DrawDebugLine(GetWorld(),
				Sp->GetLocationAtDistanceAlongSpline(((float)s/Steps)*Len,ESplineCoordinateSpace::World),
				Sp->GetLocationAtDistanceAlongSpline(((float)(s+1)/Steps)*Len,ESplineCoordinateSpace::World),
				SeaLaneDebugColor,false,DebugDuration,0,SeaLaneDebugThickness); } }
}

void AVoxelSplineNetworkGenerator::DrawDebugCoast() const
{
	for (USplineComponent* Sp : CoastSplines) { if (!Sp) continue;
		const int32 Steps=FMath::Max(Sp->GetNumberOfSplinePoints()*4,20); const float Len=Sp->GetSplineLength();
		for (int32 s=0; s<Steps; ++s) {
			DrawDebugLine(GetWorld(),
				Sp->GetLocationAtDistanceAlongSpline(((float)s/Steps)*Len,ESplineCoordinateSpace::World),
				Sp->GetLocationAtDistanceAlongSpline(((float)(s+1)/Steps)*Len,ESplineCoordinateSpace::World),
				CoastDebugColor,false,DebugDuration,0,CoastDebugThickness); } }
}

void AVoxelSplineNetworkGenerator::ClearLabelComponents()
{
	for (UTextRenderComponent* C : LabelComponents) if (C) C->DestroyComponent();
	LabelComponents.Empty();
}

void AVoxelSplineNetworkGenerator::BuildLabelComponents()
{
	for (int32 i=0; i<Cells.Num(); ++i)
	{
		const FVoxelBiomeCell& Cell = Cells[i];
		if (!Cell.bOnTerrain) continue;

		FString Label; FColor Col = FColor::White;

		if (Cell.LocationRowName != NAME_None)
		{
			const int32 Li = LocationRowNames.IndexOfByKey(Cell.LocationRowName);
			if (LocationRows.IsValidIndex(Li))
			{
				const int32 Bi    = BiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
				const FString Biome    = BiomeRows.IsValidIndex(Bi) ? BiomeRows[Bi].BiomeName : TEXT("?");
				const FString DockTag  = LocationRows[Li].bConnectsToSeaLane ? TEXT("\n[DOCK]") : TEXT("");
				const FString ForceTag = ForcePlacedBiomeCells.Contains(i) ? TEXT("\n[FORCED]") : TEXT("");
				Label = FString::Printf(TEXT("[%s]\n%s%s%s"),
					*LocationRows[Li].DisplayName, *Biome, *DockTag, *ForceTag);
				Col   = LocationRows[Li].DebugColor.ToFColor(true);
			}
		}
		else if (GenericDockCellSet.Contains(i))
		{
			Label = TEXT("[Generic Dock]");
			Col   = FColor(0, 220, 180);
		}
		else if (bShowBiomeLabelOnAllCells)
		{
			const int32 Bi = BiomeRowNames.IndexOfByKey(Cell.BiomeRowName);
			if (BiomeRows.IsValidIndex(Bi)) { Label=BiomeRows[Bi].BiomeName; Col=GetBiomeColor(Cell.BiomeRowName); }
		}

		if (Label.IsEmpty()) continue;

		UTextRenderComponent* T = NewObject<UTextRenderComponent>(this, *FString::Printf(TEXT("Label_%d"),i));
		T->SetupAttachment(GetRootComponent()); T->RegisterComponent();
		T->SetText(FText::FromString(Label));
		T->SetWorldLocation(Cell.WorldPosition + FVector(0.f, 0.f, DebugZLift+LabelZOffset));
		T->SetWorldSize(LabelWorldSize); T->SetTextRenderColor(Col);
		T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetWorldRotation(FRotator(90.f, 0.f, 180.f));
		LabelComponents.Add(T);
	}
}

void AVoxelSplineNetworkGenerator::BuildDockLabelComponents()
{
	for (int32 Di = 0; Di < DockSplines.Num(); ++Di)
	{
		USplineComponent* Sp = DockSplines[Di];
		if (!Sp) continue;

		const int32 NumPts = Sp->GetNumberOfSplinePoints();
		if (NumPts == 0) continue;
		
		FVector Centre = FVector::ZeroVector;
		for (int32 Pi = 0; Pi < NumPts; ++Pi)
			Centre += Sp->GetLocationAtSplinePoint(Pi, ESplineCoordinateSpace::World);
		Centre /= static_cast<float>(NumPts);
		Centre.Z += LabelZOffset;
		
		FString DockLabel = TEXT("Dock");
		const FString CompName = Sp->GetName();
		if (CompName.StartsWith(TEXT("Dock_")))
		{
			FString Suffix = CompName.RightChop(5);
			Suffix.ReplaceInline(TEXT("_"), TEXT(" "), ESearchCase::CaseSensitive);
			DockLabel = FString::Printf(TEXT("Dock\n%s"), *Suffix);
		}

		UTextRenderComponent* T = NewObject<UTextRenderComponent>(this,
			*FString::Printf(TEXT("DockLabel_%d"), Di));
		T->SetupAttachment(GetRootComponent());
		T->RegisterComponent();
		T->SetText(FText::FromString(DockLabel));
		T->SetWorldLocation(Centre);
		T->SetWorldSize(LabelWorldSize);
		T->SetTextRenderColor(DockDebugColor);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetWorldRotation(FRotator(90.f, 0.f, 180.f));
		LabelComponents.Add(T);
	}
}

AVoxelWorld* AVoxelSplineNetworkGenerator::ResolveVoxelWorld() const
{
	if (VoxelWorld)
	{
		UE_LOG(LogTemp, Verbose, TEXT("VoxelSplineNetworkGenerator: [ResolveVoxelWorld] Using explicitly assigned VoxelWorld '%s'."), *VoxelWorld->GetName());
		return VoxelWorld;
	}
	AVoxelWorld* Found = Cast<AVoxelWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass()));
	if (Found)
	{
		UE_LOG(LogTemp, Verbose, TEXT("VoxelSplineNetworkGenerator: [ResolveVoxelWorld] VoxelWorld field is unassigned — auto-found '%s' in the level."), *Found->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelSplineNetworkGenerator: [ResolveVoxelWorld] VoxelWorld field is unassigned AND no AVoxelWorld exists anywhere in the level. Assign one explicitly on the generator!"));
	}
	return Found;
}

bool AVoxelSplineNetworkGenerator::TraceGroundAt(float X, float Y, FVector& OutHit) const
{
	FHitResult Hit; FCollisionQueryParams P; P.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(X,Y,TraceStartZ), FVector(X,Y,TraceEndZ), ECC_WorldStatic, P))
	{ OutHit=Hit.ImpactPoint; return true; }
	return false;
}

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