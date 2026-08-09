#include "Alfheim/AlfheimVoxelWorld.h"
#include "Alfheim/VoxelSplineNetworkGenerator.h"
#include "PCGComponent.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"

namespace AlfheimVoxelWorldInternal
{
    static bool SetIntProperty(UObject* Target, FName PropertyName, int32 Value)
    {
        if (!Target) return false;
        FIntProperty* Prop = FindFProperty<FIntProperty>(Target->GetClass(), PropertyName);
        if (!Prop) return false;
        Prop->SetPropertyValue_InContainer(Target, Value);
        return true;
    }

    static bool GetFloatProperty(UObject* Target, FName PropertyName, float& OutValue)
    {
        if (!Target) return false;
        FFloatProperty* Prop = FindFProperty<FFloatProperty>(Target->GetClass(), PropertyName);
        if (!Prop) return false;
        OutValue = Prop->GetPropertyValue_InContainer(Target);
        return true;
    }
}

AAlfheimVoxelWorld::AAlfheimVoxelWorld()
{
    bReplicates = true;
}

void AAlfheimVoxelWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAlfheimVoxelWorld, EditBuffer);
    DOREPLIFETIME(AAlfheimVoxelWorld, EditBufferIndex);
}

void AAlfheimVoxelWorld::BeginPlay()
{
    Super::BeginPlay();

    if (bEnableManualMultiplayerEdits)
    {
        if (bEnableMultiplayer)
        {
            LogAndDisplay(TEXT("AlfheimVoxelWorld: bEnableManualMultiplayerEdits is on — bEnableMultiplayer should be off, they are alternative sync strategies."), true);
        }
        EditBuffer.SetNum(EditBufferSize);
    }

    if (bGenerateOnBeginPlay)
    {
        if (bRandomizeSeedOnBeginPlay)
            AlfheimSeed = FMath::Rand();
        GetWorldTimerManager().SetTimerForNextTick(this, &AAlfheimVoxelWorld::Generate);
    }
}

void AAlfheimVoxelWorld::Destroyed()
{
    if (VoxelSettleTimerHandle.IsValid())
        GetWorldTimerManager().ClearTimer(VoxelSettleTimerHandle);
    Super::Destroyed();
}

#if WITH_EDITOR
void AAlfheimVoxelWorld::PostEditChangeProperty(FPropertyChangedEvent& E)
{
    Super::PostEditChangeProperty(E);
    static const FName NAME_AlfheimSeed(TEXT("AlfheimSeed"));
    if (E.GetPropertyName() == NAME_AlfheimSeed)
        ApplySeedToTargets();
}
#endif

void AAlfheimVoxelWorld::Generate()
{
    if (!IsIdle())
    {
        LogAndDisplay(TEXT("AlfheimVoxelWorld: already running — ignored."), true);
        return;
    }

    if (bRandomizeSeedOnGenerate)
        AlfheimSeed = FMath::Rand();

    if (bAutoGenerateSplines && !SplineGenerator)
    {
        LogAndDisplay(TEXT("AlfheimVoxelWorld: SplineGenerator is null."), true);
        GenerationState = EAlfheimGenerationState::Error;
        return;
    }
    if (bAutoGeneratePCG && !PCGActor)
    {
        LogAndDisplay(TEXT("AlfheimVoxelWorld: PCGActor is null."), true);
        GenerationState = EAlfheimGenerationState::Error;
        return;
    }

    LogAndDisplay(FString::Printf(
        TEXT("AlfheimVoxelWorld: Pipeline start | Seed=%d | Delay=%.2fs"), AlfheimSeed, VoxelSettleDelay));

    bFullPipelineRunning = true;
    GenerationState      = EAlfheimGenerationState::RegeneratingVoxel;

    ApplySeedToTargets();
    RegenerateVoxelWorld();
    StartVoxelSettleTimer();
}

void AAlfheimVoxelWorld::Clear()
{
    if (VoxelSettleTimerHandle.IsValid())
        GetWorldTimerManager().ClearTimer(VoxelSettleTimerHandle);

    bFullPipelineRunning = false;
    GenerationState      = EAlfheimGenerationState::Idle;

    if (SplineGenerator)
    {
        SplineGenerator->ClearNetwork();
        LogAndDisplay(TEXT("AlfheimVoxelWorld: Splines cleared."));
    }

    if (UPCGComponent* PCGComp = ResolvePCGComponent())
    {
        PCGComp->Cleanup(true);
        LogAndDisplay(TEXT("AlfheimVoxelWorld: PCG cleaned."));
    }

    if (IsCreated())
    {
        DestroyWorld();
        LogAndDisplay(TEXT("AlfheimVoxelWorld: Voxel world destroyed."));
    }
}

void AAlfheimVoxelWorld::RandomizeSeed()
{
    AlfheimSeed = FMath::Rand();
    ApplySeedToTargets();
    LogAndDisplay(FString::Printf(TEXT("AlfheimVoxelWorld: Seed = %d"), AlfheimSeed));
}

void AAlfheimVoxelWorld::ApplySeedToTargets()
{
    CachedVoxelGenerator = Generator.GetObject();

    if (CachedVoxelGenerator &&
        AlfheimVoxelWorldInternal::SetIntProperty(CachedVoxelGenerator, TEXT("Master_Seed"), AlfheimSeed))
    {
        float Height = 0.f, Freq = 0.f;
        AlfheimVoxelWorldInternal::GetFloatProperty(CachedVoxelGenerator, TEXT("Terrain_Height_Scale"), Height);
        AlfheimVoxelWorldInternal::GetFloatProperty(CachedVoxelGenerator, TEXT("Base_Frequency"), Freq);
        LogAndDisplay(FString::Printf(
            TEXT("AlfheimVoxelWorld: Seed=%d | Height=%.1f | Freq=%.4f"),
            AlfheimSeed, Height, Freq));
    }
    else
    {
        LogAndDisplay(TEXT("AlfheimVoxelWorld: Generator is null, in Class mode, or has no Master_Seed property — assign a UVG_Alfheim *asset* (not a class) to the Generator slot."), true);
    }

    if (SplineGenerator)
        SplineGenerator->Seed = AlfheimSeed;

    if (UPCGComponent* PCGComp = ResolvePCGComponent())
        AlfheimVoxelWorldInternal::SetIntProperty(PCGComp, TEXT("Seed"), AlfheimSeed);
}

void AAlfheimVoxelWorld::SetSeed(int32 NewSeed)
{
    AlfheimSeed = NewSeed;
    ApplySeedToTargets();
}

void AAlfheimVoxelWorld::SetVoxelSettleDelay(float NewDelay)
{
    VoxelSettleDelay = FMath::Max(NewDelay, 0.f);
}

void AAlfheimVoxelWorld::SetRandomizeSeedOnGenerate(bool bEnabled)
{
    bRandomizeSeedOnGenerate = bEnabled;
}

void AAlfheimVoxelWorld::SetAutoGenerateSplines(bool bEnabled)
{
    bAutoGenerateSplines = bEnabled;
}

void AAlfheimVoxelWorld::SetAutoGeneratePCG(bool bEnabled)
{
    bAutoGeneratePCG = bEnabled;
}

void AAlfheimVoxelWorld::RegenerateVoxelWorld()
{
    if (IsCreated())
        DestroyWorld();

    CreateWorld();

    LogAndDisplay(FString::Printf(
        TEXT("AlfheimVoxelWorld: CreateWorld() called (Seed=%d). Waiting %.2fs..."),
        AlfheimSeed, VoxelSettleDelay));

    OnVoxelWorldRecreated.Broadcast();
}

void AAlfheimVoxelWorld::RegenerateSplines()
{
    if (!SplineGenerator)
    {
        LogAndDisplay(TEXT("AlfheimVoxelWorld: SplineGenerator null."), true);
        return;
    }

    GenerationState = EAlfheimGenerationState::GeneratingSplines;
    LogAndDisplay(TEXT("AlfheimVoxelWorld: Generating splines..."));

    SplineGenerator->GenerateNetworkDelayed();
    OnSplinesComplete.Broadcast();
}

void AAlfheimVoxelWorld::RegeneratePCG()
{
    UPCGComponent* PCGComp = ResolvePCGComponent();
    if (!PCGComp)
    {
        LogAndDisplay(TEXT("AlfheimVoxelWorld: No PCGComponent found — skipping."), true);
        GenerationState = EAlfheimGenerationState::Complete;
        return;
    }

    GenerationState = EAlfheimGenerationState::GeneratingPCG;
    
    LogAndDisplay(TEXT("AlfheimVoxelWorld: PCG Cleanup()..."));
    PCGComp->Cleanup(/*bRemoveComponents=*/true);

    LogAndDisplay(TEXT("AlfheimVoxelWorld: PCG Generate()..."));
    PCGComp->Generate(/*bForce=*/true);
    LogAndDisplay(TEXT("AlfheimVoxelWorld: PCG triggered."));
    OnPCGComplete.Broadcast();
}

UPCGComponent* AAlfheimVoxelWorld::ResolvePCGComponent() const
{
    return PCGActor ? PCGActor->FindComponentByClass<UPCGComponent>() : nullptr;
}

void AAlfheimVoxelWorld::StartVoxelSettleTimer()
{
    if (VoxelSettleTimerHandle.IsValid())
        GetWorldTimerManager().ClearTimer(VoxelSettleTimerHandle);

    GenerationState = EAlfheimGenerationState::WaitingForVoxel;

    if (VoxelSettleDelay <= 0.f) { OnVoxelSettleComplete(); return; }

    GetWorldTimerManager().SetTimer(
        VoxelSettleTimerHandle, this,
        &AAlfheimVoxelWorld::OnVoxelSettleComplete,
        VoxelSettleDelay, /*bLoop=*/false);
}

void AAlfheimVoxelWorld::OnVoxelSettleComplete()
{
    LogAndDisplay(FString::Printf(TEXT("AlfheimVoxelWorld: Settle complete (%.2fs)."), VoxelSettleDelay));

    if (bFullPipelineRunning && bAutoGenerateSplines) RegenerateSplines();
    if (bFullPipelineRunning && bAutoGeneratePCG)     RegeneratePCG();

    GenerationState      = EAlfheimGenerationState::Complete;
    bFullPipelineRunning = false;

    LogAndDisplay(TEXT("AlfheimVoxelWorld: Pipeline complete."));
    OnPipelineComplete.Broadcast();
}

void AAlfheimVoxelWorld::LogAndDisplay(const FString& Msg, bool bWarning) const
{
    if (bWarning)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, Msg);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Msg);
    }
}

void AAlfheimVoxelWorld::AddEdit_Implementation(float BrushSize, FVector Position, FVector Normal, bool bAlternativeMode)
{
    if (!bEnableManualMultiplayerEdits) return;
    if (EditBuffer.Num() != EditBufferSize) EditBuffer.SetNum(EditBufferSize);

    FAlfheimEditEntry Entry;
    Entry.BrushSize        = FMath::RoundToInt(BrushSize * 10.f);
    const FVector ScaledPos = Position * 10.f;
    const FVector ScaledNorm = Normal * 10.f;
    Entry.Position = FIntVector(
        FMath::TruncToInt(ScaledPos.X), FMath::TruncToInt(ScaledPos.Y), FMath::TruncToInt(ScaledPos.Z));
    Entry.Normal = FIntVector(
        FMath::TruncToInt(ScaledNorm.X), FMath::TruncToInt(ScaledNorm.Y), FMath::TruncToInt(ScaledNorm.Z));
    Entry.bAlternativeMode = bAlternativeMode;
    
    if (EditBufferIndex - LastSyncBufferIndex >= EditBufferSize)
    {
        LogAndDisplay(TEXT("AlfheimVoxelWorld: Circular Buffer Overflow! Need to increase EditBufferSize"), true);
    }

    EditBuffer[EditBufferIndex % EditBufferSize] = Entry;
    ++EditBufferIndex;

    ApplyPendingEdits();
}

void AAlfheimVoxelWorld::ApplyPendingEdits()
{
    if (!bEnableManualMultiplayerEdits) return;

    for (int32 i = LastSyncBufferIndex; i < EditBufferIndex; ++i)
    {
        const FAlfheimEditEntry& Entry = EditBuffer[i % EditBufferSize];
        SurfaceEdit(
            Entry.BrushSize / 10.f,
            FVector(Entry.Position) / 10.f,
            FVector(Entry.Normal) / 10.f,
            Entry.bAlternativeMode);
    }

    LastSyncBufferIndex = EditBufferIndex;
}

void AAlfheimVoxelWorld::SurfaceEdit(float BrushSize, FVector Position, FVector Normal, bool bAlternativeMode)
{
    LogAndDisplay(FString::Printf(
        TEXT("AlfheimVoxelWorld: SurfaceEdit(BrushSize=%.1f, Position=%s, Normal=%s, Alt=%d) — TODO: wire to actual voxel edit call"),
        BrushSize, *Position.ToString(), *Normal.ToString(), bAlternativeMode ? 1 : 0));
}