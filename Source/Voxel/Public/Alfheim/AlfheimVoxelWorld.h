#pragma once

#include "CoreMinimal.h"
#include "VoxelWorld.h"
#include "AlfheimVoxelWorld.generated.h"

class AVoxelSplineNetworkGenerator;
class UPCGComponent;

UENUM(BlueprintType)
enum class EAlfheimGenerationState : uint8
{
    Idle                UMETA(DisplayName = "Idle"),
    RegeneratingVoxel   UMETA(DisplayName = "Regenerating Voxel World"),
    WaitingForVoxel     UMETA(DisplayName = "Waiting for Voxel Collision"),
    GeneratingSplines   UMETA(DisplayName = "Generating Spline Network"),
    GeneratingPCG       UMETA(DisplayName = "Generating PCG"),
    Complete            UMETA(DisplayName = "Complete"),
    Error               UMETA(DisplayName = "Error")
};

USTRUCT(BlueprintType)
struct FAlfheimEditEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Alfheim")
    float BrushSize = 0.f;

    UPROPERTY(BlueprintReadWrite, Category = "Alfheim")
    FIntVector Position = FIntVector::ZeroValue;

    UPROPERTY(BlueprintReadWrite, Category = "Alfheim")
    FIntVector Normal = FIntVector::ZeroValue;

    UPROPERTY(BlueprintReadWrite, Category = "Alfheim")
    bool bAlternativeMode = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAlfheimVoxelComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAlfheimSplinesComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAlfheimPCGComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAlfheimPipelineComplete);

UCLASS(Blueprintable, BlueprintType, meta = (PrioritizeCategories = "Alfheim"))
class AAlfheimVoxelWorld : public AVoxelWorld
{
    GENERATED_BODY()

public:

    AAlfheimVoxelWorld();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|References")
    TObjectPtr<AVoxelSplineNetworkGenerator> SplineGenerator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|References",
        meta = (DisplayName = "PCG Actor"))
    TObjectPtr<AActor> PCGActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Settings")
    int32 AlfheimSeed = 1337;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Settings",
        meta = (DisplayName = "Randomize Seed on Generate"))
    bool bRandomizeSeedOnGenerate = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Settings|Startup",
        meta = (DisplayName = "Generate New Map On Begin Play"))
    bool bGenerateOnBeginPlay = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Settings|Startup",
        meta = (DisplayName = "Randomize Seed On Begin Play", EditCondition = "bGenerateOnBeginPlay"))
    bool bRandomizeSeedOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Settings",
        meta = (ClampMin = 0.f, DisplayName = "Post-Load Settle Buffer (seconds)"))
    float VoxelSettleDelay = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Settings",
        meta = (DisplayName = "Auto-Generate Splines"))
    bool bAutoGenerateSplines = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Settings",
        meta = (DisplayName = "Auto-Generate PCG"))
    bool bAutoGeneratePCG = true;

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Settings")
    void SetSeed(int32 NewSeed);

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Settings")
    void SetVoxelSettleDelay(float NewDelay);

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Settings")
    void SetRandomizeSeedOnGenerate(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Settings")
    void SetAutoGenerateSplines(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Settings")
    void SetAutoGeneratePCG(bool bEnabled);

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Alfheim|State")
    EAlfheimGenerationState GenerationState = EAlfheimGenerationState::Idle;

    UPROPERTY(BlueprintAssignable, Category = "Alfheim|Events")
    FOnAlfheimVoxelComplete OnVoxelWorldRecreated;

    UPROPERTY(BlueprintAssignable, Category = "Alfheim|Events")
    FOnAlfheimSplinesComplete OnSplinesComplete;

    UPROPERTY(BlueprintAssignable, Category = "Alfheim|Events")
    FOnAlfheimPCGComplete OnPCGComplete;

    UPROPERTY(BlueprintAssignable, Category = "Alfheim|Events")
    FOnAlfheimPipelineComplete OnPipelineComplete;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Alfheim")
    void Generate();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Alfheim")
    void Clear();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Alfheim", meta = (DisplayName = "Randomize Seed"))
    void RandomizeSeed();

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Stages")
    void RegenerateVoxelWorld();

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Stages")
    void RegenerateSplines();

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Stages")
    void RegeneratePCG();

    UFUNCTION(BlueprintPure, Category = "Alfheim")
    bool IsIdle() const
    {
        return GenerationState == EAlfheimGenerationState::Idle
            || GenerationState == EAlfheimGenerationState::Complete
            || GenerationState == EAlfheimGenerationState::Error;
    }

    UFUNCTION(BlueprintPure, Category = "Alfheim")
    UPCGComponent* ResolvePCGComponent() const;

    UFUNCTION(BlueprintCallable, Category = "Alfheim")
    void ApplySeedToTargets();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Multiplayer")
    bool bEnableManualMultiplayerEdits = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alfheim|Multiplayer", meta = (ClampMin = 1))
    int32 EditBufferSize = 64;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Alfheim|Multiplayer")
    TArray<FAlfheimEditEntry> EditBuffer;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Alfheim|Multiplayer")
    int32 EditBufferIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Alfheim|Multiplayer")
    int32 LastSyncBufferIndex = 0;

    UFUNCTION(BlueprintNativeEvent, Category = "Alfheim|Multiplayer")
    void AddEdit(float BrushSize, FVector Position, FVector Normal, bool bAlternativeMode);
    virtual void AddEdit_Implementation(float BrushSize, FVector Position, FVector Normal, bool bAlternativeMode);

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Multiplayer")
    void ApplyPendingEdits();

    UFUNCTION(BlueprintCallable, Category = "Alfheim|Multiplayer")
    void SurfaceEdit(float BrushSize, FVector Position, FVector Normal, bool bAlternativeMode);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

    virtual void BeginPlay() override;
    virtual void Destroyed() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

    FTimerHandle VoxelSettleTimerHandle;
    FDelegateHandle PCGCleanedHandle;
    FDelegateHandle PCGGeneratedHandle;
    bool bFullPipelineRunning = false;
    bool bVoxelStageInFlight = false;
    bool bSplinesStageInFlight = false;
    bool bPCGStageInFlight = false;

    UPROPERTY(Transient)
    TObjectPtr<UObject> CachedVoxelGenerator;

    UFUNCTION()
    void HandleVoxelWorldLoaded();

    void HandlePCGCleaned(UPCGComponent* Component);
    void HandlePCGGenerated(UPCGComponent* Component);

    void UnbindAllGenerationDelegates();

    void OnVoxelSettleComplete();
    void LogAndDisplay(const FString& Msg, bool bWarning = false) const;
};