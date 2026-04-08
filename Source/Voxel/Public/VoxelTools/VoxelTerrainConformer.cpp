// VoxelTerrainConformer.cpp
// Lost Crow Dev - https://github.com/lostcrowdev/VoxelPluginFreeLegacy-Alfheim

#include "VoxelTerrainConformer.h"
#include "VoxelTools/VoxelBlueprintLibrary.h"
#include "VoxelTools/VoxelDataTools.h"
#include "VoxelData/VoxelData.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "PCGVolume.h"
#include "PCGComponent.h"

AVoxelTerrainConformer::AVoxelTerrainConformer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AVoxelTerrainConformer::BeginPlay()
{
	Super::BeginPlay();
	
	if (!VoxelWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("No VoxelWorld assigned"));
		return;
	}
	
	if (!VoxelWorld->IsCreated())
	{
		VoxelWorld->OnWorldLoaded.AddUniqueDynamic(this, &AVoxelTerrainConformer::OnVoxelWorldLoaded);
		return;
	}
	
	OnVoxelWorldLoaded();
}

void AVoxelTerrainConformer::OnVoxelWorldLoaded()
{
	if (!VoxelWorld || !VoxelWorld->IsCreated())
	{
		UE_LOG(LogTemp, Error, TEXT("Voxel world not ready"));
		return;
	}
	
	if (bAutoProcessOnBeginPlay)
	{
		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			ProcessAllTaggedActors();
		});
	}
	
	if (bContinuousUpdate)
	{
		GetWorldTimerManager().SetTimer(
			ContinuousUpdateTimer,
			this,
			&AVoxelTerrainConformer::ProcessAllTaggedActors,
			UpdateInterval,
			true
		);
	}
}

void AVoxelTerrainConformer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TArray<AActor*> AVoxelTerrainConformer::FindEnvironmentActors()
{
	TArray<AActor*> FoundActors;
	
	if (!GetWorld())
	{
		return FoundActors;
	}
	
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(EnvironmentActorTag))
		{
			FoundActors.Add(Actor);
		}
	}
	
	return FoundActors;
}

void AVoxelTerrainConformer::ProcessAllTaggedActors()
{
	if (!VoxelWorld || !VoxelWorld->IsCreated())
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelWorld not ready"));
		return;
	}
	
	ExecuteTerrainPipeline();
}

void AVoxelTerrainConformer::ExecuteTerrainPipeline()
{
	const double StartTime = FPlatformTime::Seconds();
	AllModifiedBounds.Empty();
	CurrentPipelineStep = EPipelineStep::SplineTerraforming;
	
	if (bEnableSplineTerraforming)
	{
		ProcessSplineTerraforming();
	}
	
	CurrentPipelineStep = EPipelineStep::PCGGeneration;
	if (bGeneratePCG && PCGVolumes.Num() > 0)
	{
		ProcessPCGGeneration();
		return; // continue in callback
	}
	
	CurrentPipelineStep = EPipelineStep::DetailConforming;
	if (bEnableDetailConforming)
	{
		ProcessDetailConforming();
	}
	
	CurrentPipelineStep = EPipelineStep::Complete;
	
	if (AllModifiedBounds.Num() > 0)
	{
		FVoxelIntBox MergedBounds = AllModifiedBounds[0];
		for (int32 i = 1; i < AllModifiedBounds.Num(); i++)
		{
			MergedBounds = MergedBounds + AllModifiedBounds[i];
		}
		
		UVoxelBlueprintLibrary::UpdateBounds(VoxelWorld, MergedBounds);
		const double ElapsedTime = FPlatformTime::Seconds() - StartTime;
		UE_LOG(LogTemp, Warning, TEXT("Pipeline finished in %.2f s"), ElapsedTime);
	}
	
	AllModifiedBounds.Empty();
}

void AVoxelTerrainConformer::ProcessSplineTerraforming()
{
	if (!VoxelWorld || !VoxelWorld->IsCreated())
	{
		return;
	}
	
	TArray<USplineComponent*> Splines = FindFlattenSplines();
	if (Splines.Num() == 0)
	{
		return;
	}
	
	for (USplineComponent* Spline : Splines)
	{
		FlattenTerrainInSpline(Spline);
	}
}

void AVoxelTerrainConformer::ProcessPCGGeneration()
{
	if (!bGeneratePCG || PCGVolumes.Num() == 0)
	{
		return;
	}
	
	CompletedPCGComponents.Empty();
	TotalPCGComponentsToWait = 0;
	
	for (APCGVolume* Volume : PCGVolumes)
	{
		if (!Volume)
		{
			continue;
		}
		
		// Get the PCG component from the volume via FindComponentByClass
		UPCGComponent* PCGComponent = Volume->FindComponentByClass<UPCGComponent>();
		if (!PCGComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("PCG volume has no PCGComponent: %s"), *Volume->GetName());
			continue;
		}
		
		// Ensure we have only one binding: remove existing binds for this object then add ours
		PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
		PCGComponent->OnPCGGraphGeneratedDelegate.AddUObject(this, &AVoxelTerrainConformer::OnPCGVolumeCompleted);
		
		TotalPCGComponentsToWait++;
		
		// Trigger generation
		UE_LOG(LogTemp, Warning, TEXT("Generating PCG for volume: %s"), *Volume->GetName());
		PCGComponent->Generate();
	}
	
	if (TotalPCGComponentsToWait == 0)
	{
		CurrentPipelineStep = EPipelineStep::DetailConforming;
		ProcessDetailConforming();
	}
}

void AVoxelTerrainConformer::OnPCGVolumeCompleted(UPCGComponent* PCGComponent)
{
	if (!PCGComponent)
	{
		return;
	}
	
	// Mark component completed
	CompletedPCGComponents.Add(PCGComponent);
	
	UE_LOG(LogTemp, Warning, TEXT("PCG component finished (%d/%d)"), CompletedPCGComponents.Num(), TotalPCGComponentsToWait);
	
	if (AreAllPCGVolumesComplete())
	{
		// Unbind from all PCG components attached to the listed volumes
		for (APCGVolume* Volume : PCGVolumes)
		{
			if (!Volume)
			{
				continue;
			}
			
			UPCGComponent* Comp = Volume->FindComponentByClass<UPCGComponent>();
			if (Comp)
			{
				Comp->OnPCGGraphGeneratedDelegate.RemoveAll(this);
			}
		}
		
		CurrentPipelineStep = EPipelineStep::DetailConforming;
		ProcessDetailConforming();
		
		CurrentPipelineStep = EPipelineStep::Complete;
		
		if (AllModifiedBounds.Num() > 0)
		{
			FVoxelIntBox MergedBounds = AllModifiedBounds[0];
			for (int32 i = 1; i < AllModifiedBounds.Num(); i++)
			{
				MergedBounds = MergedBounds + AllModifiedBounds[i];
			}
			
			UVoxelBlueprintLibrary::UpdateBounds(VoxelWorld, MergedBounds);
			UE_LOG(LogTemp, Warning, TEXT("Pipeline complete after PCG"));
		}
		
		AllModifiedBounds.Empty();
	}
}

bool AVoxelTerrainConformer::AreAllPCGVolumesComplete() const
{
	return CompletedPCGComponents.Num() >= TotalPCGComponentsToWait;
}

void AVoxelTerrainConformer::ProcessDetailConforming()
{
	if (!VoxelWorld || !VoxelWorld->IsCreated())
	{
		return;
	}
	
	TArray<AActor*> Actors = FindEnvironmentActors();
	if (Actors.Num() == 0)
	{
		return;
	}
	
	if (bContinuousUpdate)
	{
		TArray<AActor*> MovedActors;
		for (AActor* Actor : Actors)
		{
			if (HasActorMoved(Actor))
			{
				MovedActors.Add(Actor);
				UpdateActorPosition(Actor);
			}
		}
		
		if (MovedActors.Num() > 0)
		{
			ProcessActorBatch(MovedActors);
		}
	}
	else
	{
		ProcessActorBatch(Actors);
	}
}

void AVoxelTerrainConformer::ProcessActorBatch(const TArray<AActor*>& Actors)
{
	if (Actors.Num() == 0 || !VoxelWorld || !VoxelWorld->IsCreated())
	{
		return;
	}
	
	if (bUseTwoPassSystem)
	{
		TArray<FVector> AllConformPoints;
		for (AActor* Actor : Actors)
		{
			TArray<FVector> ActorPoints = GetConformPointsForActor(Actor);
			AllConformPoints.Append(ActorPoints);
		}
		
		if (AllConformPoints.Num() > 0)
		{
			ApplyLargeScaleSlopes(AllConformPoints);
		}
		
		for (AActor* Actor : Actors)
		{
			ProcessSingleActor(Actor);
		}
	}
	else
	{
		for (AActor* Actor : Actors)
		{
			ProcessSingleActor(Actor);
		}
	}
}

TArray<FVector> AVoxelTerrainConformer::GetConformPointsForActor(AActor* Actor)
{
	TArray<FVector> ConformPoints;
	if (!Actor)
	{
		return ConformPoints;
	}
	
	TArray<AActor*> ChildActors;
	Actor->GetAttachedActors(ChildActors);
	for (AActor* ChildActor : ChildActors)
	{
		if (ChildActor && ChildActor->ActorHasTag(ConformPointTag))
		{
			ConformPoints.Add(ChildActor->GetActorLocation());
		}
	}
	
	TArray<USceneComponent*> Components;
	Actor->GetComponents<USceneComponent>(Components);
	for (USceneComponent* Component : Components)
	{
		if (Component && Component != Actor->GetRootComponent())
		{
			if (Component->ComponentHasTag(ConformPointTag))
			{
				ConformPoints.Add(Component->GetComponentLocation());
			}
		}
	}
	
	if (ConformPoints.Num() == 0 && !bRequireConformPoints)
	{
		ConformPoints.Add(Actor->GetActorLocation());
	}
	
	return ConformPoints;
}

void AVoxelTerrainConformer::DrawDebugVisualization(
	AActor* Actor,
	const FVector& PointLocation,
	const FVector& SurfaceLocation,
	const FVoxelIntBox& EditBounds,
	const FBox& ActorWorldBounds,
	int32 PointIndex,
	int32 TotalPoints)
{
	if (!GetWorld() || DebugLifetime <= 0.0f)
	{
		return;
	}
	
	const float Thickness = 3.0f;
	const float SphereSize = 50.0f;
	FColor PointColor = FColor::MakeRedToGreenColorFromScalar(float(PointIndex) / FMath::Max(1.0f, float(TotalPoints - 1)));
	
	DrawDebugSphere(GetWorld(), PointLocation, SphereSize, 12, PointColor, false, DebugLifetime, 0, Thickness);
	DrawDebugSphere(GetWorld(), SurfaceLocation, SphereSize, 12, FColor::Green, false, DebugLifetime, 0, Thickness);
	DrawDebugLine(GetWorld(), PointLocation, SurfaceLocation, FColor::Yellow, false, DebugLifetime, 0, Thickness);
	DrawDebugSphere(GetWorld(), SurfaceLocation, EditRadius, 24, FColor::Red, false, DebugLifetime, 0, 2.0f);
	
	FVector VoxelBoundsMin = VoxelWorld->LocalToGlobal(EditBounds.Min);
	FVector VoxelBoundsMax = VoxelWorld->LocalToGlobal(EditBounds.Max);
	FBox VoxelWorldBox(VoxelBoundsMin, VoxelBoundsMax);
	
	DrawDebugBox(GetWorld(), VoxelWorldBox.GetCenter(), VoxelWorldBox.GetExtent(), FColor::Magenta, false, DebugLifetime, 0, 2.0f);
	
	if (PointIndex == 0)
	{
		DrawDebugBox(GetWorld(), ActorWorldBounds.GetCenter(), ActorWorldBounds.GetExtent(), FColor::Cyan, false, DebugLifetime, 0, Thickness);
	}
}

void AVoxelTerrainConformer::ApplyLargeScaleSlopes(const TArray<FVector>& AllConformPoints)
{
	if (AllConformPoints.Num() == 0 || !VoxelWorld || !VoxelWorld->IsCreated())
	{
		return;
	}
	
	VOXEL_FUNCTION_COUNTER();
	
	FBox TotalBounds(ForceInit);
	for (const FVector& Point : AllConformPoints)
	{
		TotalBounds += Point;
	}
	
	TotalBounds = TotalBounds.ExpandBy(FVector(LargeSlopeRadius, LargeSlopeRadius, LargeSlopeRadius * 0.5f));
	
	FVoxelIntBox VoxelBounds = UVoxelBlueprintLibrary::MakeIntBoxFromGlobalPositionAndRadius(
		VoxelWorld,
		TotalBounds.GetCenter(),
		TotalBounds.GetExtent().GetMax()
	);
	
	int32 ModifiedCount = 0;
	
	VoxelBounds.Iterate([&](int32 X, int32 Y, int32 Z)
	{
		const FIntVector CurrentVoxel(X, Y, Z);
		const FVector VoxelWorldPos = VoxelWorld->LocalToGlobal(CurrentVoxel);
		
		float WeightedTargetHeight = 0.0f;
		float TotalWeight = 0.0f;
		
		for (const FVector& ConformPoint : AllConformPoints)
		{
			const FVector2D VoxelXY(VoxelWorldPos.X, VoxelWorldPos.Y);
			const FVector2D PointXY(ConformPoint.X, ConformPoint.Y);
			const float HorizontalDist = FVector2D::Distance(VoxelXY, PointXY);
			
			if (HorizontalDist < LargeSlopeRadius)
			{
				const float NormalizedDist = HorizontalDist / LargeSlopeRadius;
				float Weight = 1.0f - NormalizedDist;
				Weight = FMath::Pow(Weight, 4.0f);
				Weight = FMath::SmoothStep(0.0f, 1.0f, Weight);
				
				WeightedTargetHeight += ConformPoint.Z * Weight;
				TotalWeight += Weight;
			}
		}
		
		if (TotalWeight < 0.01f)
		{
			return;
		}
		
		const float TargetHeight = WeightedTargetHeight / TotalWeight;
		const float HeightDifference = VoxelWorldPos.Z - TargetHeight;
		const float DistFromSurface = HeightDifference / VoxelWorld->VoxelSize;
		
		float NewValue = 0.0f;
		if (DistFromSurface > 2.0f)
		{
			NewValue = FMath::Clamp(DistFromSurface * 0.2f, 0.0f, 5.0f);
		}
		else if (DistFromSurface < -2.0f)
		{
			NewValue = FMath::Clamp(DistFromSurface * 0.2f, -5.0f, 0.0f);
		}
		else
		{
			NewValue = DistFromSurface * 0.2f;
		}
		
		NewValue *= LargeSlopeStrength * (TotalWeight / AllConformPoints.Num());
		
		if (FMath::Abs(NewValue) > 0.05f)
		{
			UVoxelDataTools::SetValue(VoxelWorld, CurrentVoxel, NewValue);
			ModifiedCount++;
		}
	});
	
	AllModifiedBounds.Add(VoxelBounds);
}

FIntVector AVoxelTerrainConformer::FindNearestTerrainSurface(const FIntVector& StartVoxel, int32 SearchRadius)
{
	// Simple placeholder for surface search; returns start voxel
	return StartVoxel;
}

void AVoxelTerrainConformer::ProcessSingleActor(AActor* Actor)
{
	if (!Actor || !VoxelWorld || !VoxelWorld->IsCreated())
	{
		return;
	}
	
	FVector BoundsOrigin, BoundsExtent;
	Actor->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	FBox ActorWorldBounds(BoundsOrigin - BoundsExtent, BoundsOrigin + BoundsExtent);
	
	TArray<FVector> ConformPoints = GetConformPointsForActor(Actor);
	if (ConformPoints.Num() == 0 && bRequireConformPoints)
	{
		return;
	}
	
	for (int32 i = 0; i < ConformPoints.Num(); i++)
	{
		const FVector& PointLocation = ConformPoints[i];
		
		FIntVector PointVoxel = VoxelWorld->GlobalToLocal(PointLocation);
		const int32 SearchRadius = FMath::Max(100, FMath::CeilToInt(EditRadius / VoxelWorld->VoxelSize) * 3);
		FIntVector SurfaceVoxel = FindNearestTerrainSurface(PointVoxel, SearchRadius);
		FVector SurfaceWorldLocation = VoxelWorld->LocalToGlobal(SurfaceVoxel);
		
		FVector TargetLocation = SurfaceWorldLocation + FVector(0, 0, TerrainOffset);
		
		const float TotalRadius = (SlopeDistance > 0) ? SlopeDistance : EditRadius;
		const float SearchRadiusWorld = TotalRadius + BoundsPadding;
		
		FVoxelIntBox EditBounds = UVoxelBlueprintLibrary::MakeIntBoxFromGlobalPositionAndRadius(
			VoxelWorld,
			TargetLocation,
			SearchRadiusWorld
		);
		
		DrawDebugVisualization(Actor, PointLocation, TargetLocation, EditBounds, ActorWorldBounds, i, ConformPoints.Num());
		ConformTerrainAtLocation(TargetLocation, EditBounds);
		AllModifiedBounds.Add(EditBounds);
	}
}

FVoxelIntBox AVoxelTerrainConformer::GetActorVoxelBounds(AActor* Actor)
{
	if (!Actor || !VoxelWorld)
	{
		return FVoxelIntBox();
	}
	
	FVector Origin, Extent;
	Actor->GetActorBounds(false, Origin, Extent);
	Extent += FVector(BoundsPadding);
	
	FVector MinWorld = Origin - Extent;
	FVector MaxWorld = Origin + Extent;
	
	FIntVector MinVoxel = VoxelWorld->GlobalToLocal(MinWorld);
	FIntVector MaxVoxel = VoxelWorld->GlobalToLocal(MaxWorld);
	
	FVoxelIntBox Bounds(MinVoxel, MaxVoxel);
	const int32 RadiusInVoxels = FMath::CeilToInt(EditRadius / VoxelWorld->VoxelSize);
	Bounds = Bounds.Extend(RadiusInVoxels);
	
	return Bounds;
}

void AVoxelTerrainConformer::ConformTerrainAtLocation(const FVector& Location, const FVoxelIntBox& Bounds)
{
	if (!VoxelWorld || !VoxelWorld->IsCreated())
	{
		return;
	}
	
	VOXEL_FUNCTION_COUNTER();
	
	const float ConformPointZ = Location.Z;
	const float VoxelSize = VoxelWorld->VoxelSize;
	
	// Sample nearby heights
	TArray<float> SurroundingHeights;
	const int32 NumSamples = 16;
	const float SampleRadius = SlopeDistance * 0.8f;
	
	for (int32 i = 0; i < NumSamples; i++)
	{
		const float Angle = (float)i / NumSamples * 2.0f * PI;
		const FVector SamplePos = Location + FVector(
			FMath::Cos(Angle) * SampleRadius,
			FMath::Sin(Angle) * SampleRadius,
			0.0f
		);
		
		for (float TestZ = ConformPointZ + 500.0f; TestZ > ConformPointZ - 500.0f; TestZ -= VoxelSize)
		{
			const FIntVector TestVoxel = VoxelWorld->GlobalToLocal(FVector(SamplePos.X, SamplePos.Y, TestZ));
			
			float Value = 0.0f;
			UVoxelDataTools::GetValue(Value, VoxelWorld, TestVoxel);
			
			if (Value > -0.1f && Value < 0.1f)
			{
				SurroundingHeights.Add(TestZ);
				break;
			}
		}
	}
	
	float AverageSurroundingHeight = ConformPointZ;
	if (SurroundingHeights.Num() > 0)
	{
		float Sum = 0.0f;
		for (float H : SurroundingHeights)
		{
			Sum += H;
		}
		AverageSurroundingHeight = Sum / SurroundingHeights.Num();
	}
	
	int32 ModifiedCount = 0;
	
	const float SlopeRadians = FMath::DegreesToRadians(FMath::Clamp(SlopeAngle, 5.0f, 45.0f));
	const float SlopeMultiplier = FMath::Tan(SlopeRadians);
	const float VoxelSlopeStrength = SlopeMultiplier * 0.3f;
	
	Bounds.Iterate([&](int32 X, int32 Y, int32 Z)
	{
		const FIntVector CurrentVoxel(X, Y, Z);
		const FVector VoxelWorldPos = VoxelWorld->LocalToGlobal(CurrentVoxel);
		
		const FVector2D PointXY(Location.X, Location.Y);
		const FVector2D VoxelXY(VoxelWorldPos.X, VoxelWorldPos.Y);
		const float HorizontalDist = FVector2D::Distance(PointXY, VoxelXY);
		
		if (HorizontalDist > SlopeDistance)
		{
			return;
		}
		
		const float DistanceRatio = HorizontalDist / SlopeDistance;
		float Blend = 1.0f - DistanceRatio;
		Blend = FMath::Pow(Blend, 3.0f);
		Blend = FMath::SmoothStep(0.0f, 1.0f, Blend);
		
		const float TargetHeight = FMath::Lerp(AverageSurroundingHeight, ConformPointZ, Blend);
		const float HeightDifference = VoxelWorldPos.Z - TargetHeight;
		const float DistFromSurface = HeightDifference / VoxelSize;
		
		float NewValue = 0.0f;
		
		if (DistFromSurface > 2.0f)
		{
			NewValue = FMath::Clamp(DistFromSurface * VoxelSlopeStrength, 0.0f, 10.0f);
		}
		else if (DistFromSurface < -2.0f)
		{
			NewValue = FMath::Clamp(DistFromSurface * VoxelSlopeStrength, -10.0f, 0.0f);
		}
		else
		{
			NewValue = DistFromSurface * VoxelSlopeStrength;
		}
		
		NewValue *= Blend;
		
		if (FMath::Abs(NewValue) > 0.01f)
		{
			UVoxelDataTools::SetValue(VoxelWorld, CurrentVoxel, NewValue);
			ModifiedCount++;
		}
	});
	
	// Note: ModifiedCount is available for logging if needed
}

bool AVoxelTerrainConformer::HasActorMoved(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}
	
	TWeakObjectPtr<AActor> WeakActor(Actor);
	FVector CurrentLocation = Actor->GetActorLocation();
	
	if (FVector* LastPosition = LastKnownPositions.Find(WeakActor))
	{
		return !CurrentLocation.Equals(*LastPosition, 1.0f);
	}
	
	return true;
}

void AVoxelTerrainConformer::UpdateActorPosition(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}
	
	TWeakObjectPtr<AActor> WeakActor(Actor);
	LastKnownPositions.FindOrAdd(WeakActor) = Actor->GetActorLocation();
}

void AVoxelTerrainConformer::ResetContinuousUpdateTracking()
{
	LastKnownPositions.Empty();
}

TArray<USplineComponent*> AVoxelTerrainConformer::FindFlattenSplines()
{
	TArray<USplineComponent*> FoundSplines;
	
	if (!bEnableSplineTerraforming)
	{
		return FoundSplines;
	}
	
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(FlattenSplineTag))
		{
			TArray<USplineComponent*> SplineComponents;
			Actor->GetComponents<USplineComponent>(SplineComponents);
			
			for (USplineComponent* Spline : SplineComponents)
			{
				if (Spline && Spline->IsClosedLoop())
				{
					FoundSplines.Add(Spline);
				}
			}
		}
	}
	
	return FoundSplines;
}

bool AVoxelTerrainConformer::IsPointInSpline(const FVector2D& Point, USplineComponent* Spline)
{
	if (!Spline || !Spline->IsClosedLoop())
	{
		return false;
	}
	
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	int32 Crossings = 0;
	
	for (int32 i = 0; i < NumPoints; i++)
	{
		const FVector P1 = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		const FVector P2 = Spline->GetLocationAtSplinePoint((i + 1) % NumPoints, ESplineCoordinateSpace::World);
		
		const FVector2D V1(P1.X, P1.Y);
		const FVector2D V2(P2.X, P2.Y);
		
		if (((V1.Y <= Point.Y) && (V2.Y > Point.Y)) || ((V1.Y > Point.Y) && (V2.Y <= Point.Y)))
		{
			const float XIntersect = V1.X + (Point.Y - V1.Y) / (V2.Y - V1.Y) * (V2.X - V1.X);
			if (Point.X < XIntersect)
			{
				Crossings++;
			}
		}
	}
	
	return (Crossings % 2) == 1;
}

float AVoxelTerrainConformer::GetExistingVoxelValue(const FIntVector& VoxelPos)
{
	if (!VoxelWorld || !VoxelWorld->IsCreated())
	{
		return 0.0f;
	}
	
	// Simple estimate based on distance from surface
	const FVector WorldPos = VoxelWorld->LocalToGlobal(VoxelPos);
	const float EstimatedSurfaceZ = WorldPos.Z;
	const float DistanceFromSurface = WorldPos.Z - EstimatedSurfaceZ;
	
	if (DistanceFromSurface < -VoxelWorld->VoxelSize)
	{
		return -5.0f;
	}
	else if (DistanceFromSurface > VoxelWorld->VoxelSize)
	{
		return 5.0f;
	}
	else
	{
		return DistanceFromSurface / VoxelWorld->VoxelSize * 5.0f;
	}
}

void AVoxelTerrainConformer::FlattenTerrainInSpline(USplineComponent* Spline)
{
	if (!Spline || !VoxelWorld || !VoxelWorld->IsCreated())
	{
		return;
	}
	
	VOXEL_FUNCTION_COUNTER();
	
	FBox SplineBounds = Spline->Bounds.GetBox();
	const FVector2D SplineCenter(SplineBounds.GetCenter().X, SplineBounds.GetCenter().Y);
	const float SplineRadius = FMath::Max(SplineBounds.GetExtent().X, SplineBounds.GetExtent().Y);
	
	TArray<float> SurfaceHeights;
	const int32 NumSamples = 20;
	
	for (int32 i = 0; i < NumSamples; i++)
	{
		const float Angle = (float)i / NumSamples * 2.0f * PI;
		const float Dist = FMath::FRandRange(0.0f, SplineRadius * 0.8f);
		const FVector2D SamplePoint = SplineCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Dist;
		
		if (IsPointInSpline(SamplePoint, Spline))
		{
			const FVector StartPos(SamplePoint.X, SamplePoint.Y, SplineBounds.Max.Z + 1000.0f);
			
			for (float TestZ = StartPos.Z; TestZ > SplineBounds.Min.Z - 5000.0f; TestZ -= VoxelWorld->VoxelSize)
			{
				if (TestZ < 500.0f && TestZ > -500.0f)
				{
					SurfaceHeights.Add(TestZ);
					break;
				}
			}
		}
	}
	
	float TargetHeight = 0.0f;
	if (SurfaceHeights.Num() > 0)
	{
		for (float H : SurfaceHeights)
		{
			TargetHeight += H;
		}
		TargetHeight /= SurfaceHeights.Num();
	}
	else
	{
		TargetHeight = (FlattenHeight != 0.0f) ? FlattenHeight : 0.0f;
	}
	
	const FVector2D SplineCenterXY(SplineBounds.GetCenter().X, SplineBounds.GetCenter().Y);
	const float MaxInfluenceRadius = SplineRadius + FlattenEdgeFalloff;
	
	FBox ProcessBounds = SplineBounds;
	ProcessBounds = ProcessBounds.ExpandBy(FVector(MaxInfluenceRadius, MaxInfluenceRadius, 3000.0f));
	
	FVoxelIntBox VoxelBounds = UVoxelBlueprintLibrary::MakeIntBoxFromGlobalPositionAndRadius(
		VoxelWorld,
		ProcessBounds.GetCenter(),
		ProcessBounds.GetExtent().GetMax()
	);
	
	int32 ModifiedCount = 0;
	
	VoxelBounds.Iterate([&](int32 X, int32 Y, int32 Z)
	{
		const FIntVector CurrentVoxel(X, Y, Z);
		const FVector VoxelWorldPos = VoxelWorld->LocalToGlobal(CurrentVoxel);
		const FVector2D VoxelXY(VoxelWorldPos.X, VoxelWorldPos.Y);
		
		const bool bInsideSpline = IsPointInSpline(VoxelXY, Spline);
		
		const float DistFromCenter = FVector2D::Distance(VoxelXY, SplineCenterXY);
		
		const float ClosestKey = Spline->FindInputKeyClosestToWorldLocation(FVector(VoxelXY.X, VoxelXY.Y, 0));
		const FVector ClosestPoint = Spline->GetLocationAtSplineInputKey(ClosestKey, ESplineCoordinateSpace::World);
		const float DistanceToEdge = FVector2D::Distance(VoxelXY, FVector2D(ClosestPoint.X, ClosestPoint.Y));
		
		float InfluenceStrength = 0.0f;
		
		if (bInsideSpline)
		{
			if (FlattenEdgeFalloff > 0 && DistanceToEdge < FlattenEdgeFalloff)
			{
				InfluenceStrength = DistanceToEdge / FlattenEdgeFalloff;
				InfluenceStrength = FMath::Pow(InfluenceStrength, 2.0f);
			}
			else
			{
				InfluenceStrength = 1.0f;
			}
		}
		else
		{
			if (FlattenEdgeFalloff > 0 && DistanceToEdge < FlattenEdgeFalloff)
			{
				InfluenceStrength = 1.0f - (DistanceToEdge / FlattenEdgeFalloff);
				InfluenceStrength = FMath::Pow(InfluenceStrength, 2.0f);
			}
			else
			{
				return;
			}
		}
		
		if (InfluenceStrength < 0.01f)
		{
			return;
		}
		
		const float HeightDifference = VoxelWorldPos.Z - TargetHeight;
		const float DistInVoxels = HeightDifference / VoxelWorld->VoxelSize;
		
		float TargetValue = 0.0f;
		if (FMath::Abs(DistInVoxels) < 0.5f)
		{
			TargetValue = DistInVoxels * 0.1f;
		}
		else if (DistInVoxels > 0.5f)
		{
			const float DistRatio = FMath::Clamp(DistInVoxels / 10.0f, 0.0f, 1.0f);
			TargetValue = FMath::Lerp(0.05f, 2.0f, DistRatio);
		}
		else
		{
			const float DistRatio = FMath::Clamp(-DistInVoxels / 10.0f, 0.0f, 1.0f);
			TargetValue = FMath::Lerp(-0.05f, -2.0f, DistRatio);
		}
		
		const float FinalValue = TargetValue * InfluenceStrength * FlattenStrength * 0.15f;
		
		if (FMath::Abs(FinalValue) > 0.005f)
		{
			UVoxelDataTools::SetValue(VoxelWorld, CurrentVoxel, FinalValue);
			ModifiedCount++;
		}
	});
	
	AllModifiedBounds.Add(VoxelBounds);
}

void AVoxelTerrainConformer::DebugPrintSystemStatus()
{
	UE_LOG(LogTemp, Warning, TEXT("VOXEL TERRAIN CONFORMER STATUS"));
	
	if (!VoxelWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelWorld not assigned"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelWorld: %s"), *VoxelWorld->GetName());
		if (VoxelWorld->IsCreated())
		{
			UE_LOG(LogTemp, Warning, TEXT("VoxelSize: %.2f"), VoxelWorld->VoxelSize);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("VoxelWorld not created"));
		}
	}
	
	TArray<AActor*> Actors = FindEnvironmentActors();
	UE_LOG(LogTemp, Warning, TEXT("Actors with tag '%s': %d"), *EnvironmentActorTag.ToString(), Actors.Num());
	
	UE_LOG(LogTemp, Warning, TEXT("Settings: TerrainOffset %.1f, EditRadius %.1f, SlopeAngle %.1f, SlopeDistance %.1f"),
		TerrainOffset, EditRadius, SlopeAngle, SlopeDistance);
}

void AVoxelTerrainConformer::DebugCheckTerrainNearActor(AActor* Actor)
{
	if (!Actor || !VoxelWorld || !VoxelWorld->IsCreated())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid parameters for terrain check"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Terrain check: %s at %s"), *Actor->GetName(), *Actor->GetActorLocation().ToString());
}
