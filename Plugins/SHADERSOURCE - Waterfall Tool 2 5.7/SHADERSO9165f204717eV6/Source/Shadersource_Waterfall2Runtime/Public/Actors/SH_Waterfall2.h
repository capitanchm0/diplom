// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EditorComponents/SH_WaterfallGenerationEnums.h"
#include "Generation/SH_WaterfallBuilder_Path.h"
#include "Generation/SH_WaterfallBuilder_Mesh.h"
#include "Generation/SH_WaterfallBuilder_Static.h"
#include "SH_Waterfall2.generated.h"

class USH_WaterfallPathComponent;
class USH_WaterfallMeshComponent;
class USH_WaterfallSettingsComponent;
class USH_WaterfallVFXComponent;
class USH_WaterfallSFXComponent;
class ISH_WaterfallFxComponent;
class UBoxComponent;
class USceneComponent;
struct FSH_VfxPointData;

DECLARE_DELEGATE_TwoParams(Del_EnableUI, bool, FSimpleDelegate);

/* The main Waterfall actor for SHADERSOURCE Waterfall Tool 2. */
UCLASS(meta = (DisplayName = "SHADERSOURCE Waterfall 2"))
class SHADERSOURCE_WATERFALL2RUNTIME_API ASH_Waterfall2 : public AActor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootComp = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* FxAttachPoint = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallVFXComponent* VFX_Top = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallVFXComponent* VFX_Middle = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallVFXComponent* VFX_Bottom = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallSFXComponent* SFX_Top = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallSFXComponent* SFX_Middle = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallSFXComponent* SFX_Bottom = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* BakedMeshComp = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallMeshComponent* MeshComp_Singular = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallMeshComponent* MeshComp_PerPath = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallMeshComponent* MeshComp_Cross = nullptr;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallMeshComponent* MeshComp_Splash = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USplineComponent* TopSpline = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* KillPlaneVisualiser = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USH_WaterfallSettingsComponent* WaterfallSettingsComp = nullptr;
#endif

public:
	ASH_Waterfall2();

	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	void UpdateArrowComponents();
#endif

	virtual void Tick(float DeltaTime) override;
	//We need it to tick in the Level Editor for generation purposes
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	void SetStaticMesh(UStaticMesh* NewMesh);

	UStaticMeshComponent* GetBakedMeshComp() { return BakedMeshComp; }

	//If this is true, the dynamic meshes will be visible in game, if false the static mesh will be visible in game
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WaterfallSettings")
	bool bShowDynamicMeshesInGame = true;

#if WITH_EDITOR
	USH_WaterfallSettingsComponent* GetWaterfallSettings() { return WaterfallSettingsComp; }

	USplineComponent* GetTopSpline() { return TopSpline; }
	UStaticMeshComponent* GetKillPlane() { return KillPlaneVisualiser; }

	TArray<USH_WaterfallPathComponent*> GetAllPathComponents();
	USH_WaterfallPathComponent* GetPathComponent(int32 PathIndex);
	int32 NumPaths();
	virtual bool HasGeneratedPaths();
	virtual void SetPathsVisibility(bool bVisible);

	virtual bool HasGeneratedMeshes();
#endif
	TArray<USH_WaterfallMeshComponent*> GetAllMeshComponents();
	virtual void SetMeshesVisibility(bool bShowDynamicMeshes);
	void ClearDynamicMeshes(ESH_MeshGenerationType MeshType = ESH_MeshGenerationType::W2_MG_All);
#if WITH_EDITOR

	USH_WaterfallMeshComponent* GetMeshForType(ESH_MeshGenerationType MeshType);

	USceneComponent* GetFxAttachPoint() { return FxAttachPoint; }

#endif
	TArray<ISH_WaterfallFxComponent*> GetAllComponents_FX(bool bIncludeGlobals = false);
	TArray<USH_WaterfallVFXComponent*> GetAllComponents_VFX(bool bIncludeGlobals = false);
	TArray<USH_WaterfallSFXComponent*> GetAllComponents_SFX(bool bIncludeGlobals = false);
#if WITH_EDITOR
	TArray<ISH_WaterfallFxComponent*> GetComponents_FX(int32 PathIndex, int32 PointIndex);
	void DeleteComponent_FX(ISH_WaterfallFxComponent* CompToDelete, bool bRefreshDetailsPanel = false);
	void DeleteAllComponents_FX(bool bRefreshDetailsPanel = false, bool bClearGlobals = false);
	virtual void SetFxVisibility(bool bShowFx);

	//VFX-----------------------------------------------------------------------------
	USH_WaterfallVFXComponent* GetVFX_Top() { return VFX_Top; }
	USH_WaterfallVFXComponent* GetVFX_Middle() { return VFX_Middle; }
	USH_WaterfallVFXComponent* GetVFX_Bottom() { return VFX_Bottom; }

	/*@param bIncludeGlobals = whether to include the Top, Bottom, and Middle FX components too*/
	USH_WaterfallVFXComponent* AddComponent_VFX(int32 PathIndex, int32 PointIndex, FTransform ComponentTransform, bool bRefreshDetailsPanel = false);
	//--------------------------------------------------------------------------------

	//SFX-----------------------------------------------------------------------------
	USH_WaterfallSFXComponent* GetSFX_Top() { return SFX_Top; }
	USH_WaterfallSFXComponent* GetSFX_Middle() { return SFX_Middle; }
	USH_WaterfallSFXComponent* GetSFX_Bottom() { return SFX_Bottom; }

	/*@param bIncludeGlobals = whether to include the Top, Bottom, and Mid SFX components too*/
	USH_WaterfallSFXComponent* AddComponent_SFX(int32 PathIndex, int32 PointIndex, FTransform ComponentTransform, bool bRefreshDetailsPanel = false);
	//--------------------------------------------------------------------------------

	virtual void RefreshDetailsPanel();

	Del_EnableUI OnEnableUI;

private:
	FSH_WaterfallBuilder_Path PathBuilder;
	FSH_WaterfallBuilder_Mesh MeshBuilder;
	FSH_WaterfallBuilder_Static StaticBuilder;

public:
	FSH_WaterfallBuilder_Path& GetPathBuilder() { return PathBuilder; }
	FSH_WaterfallBuilder_Mesh& GetMeshBuilder() { return MeshBuilder; }
	FSH_WaterfallBuilder_Static& GetStaticBuilder() { return StaticBuilder; }
	bool IsProcessing();

	//Returns the start VfxData of all paths in world space
	virtual TArray<FSH_VfxPointData> GetAllStartVfxData();
	//Returns the end VfxData of all paths in world space
	virtual TArray<FSH_VfxPointData> GetAllEndVfxData();
	/*Get all VfxData of all paths within this z-range in world space
	(ie, DistanceRange = {-10.f,30.f} will return all VfxData in all paths whose z-value is in that range)
	This will always return at least two points for each path if any of the path is in this distance range
	and will remap to the number of RemapPoints if the value is greater than 2*/
	virtual TArray<FSH_VfxPointData> GetAllVfxDataInRange(FVector2D DistanceRange, int32 RemapPoints = 1);
	/*Get all VfxData of all paths within this point range in world space
	(ie, PointRange = {3,7} will return all VfxData between indexes 3 and 7 inclusive in all paths)*/
	virtual TArray<FSH_VfxPointData> GetAllVfxDataInRange(FIntVector2 PointRange);
#endif
	virtual void OnStaticMeshPropChange();

#if WITH_EDITOR

protected:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	void DeferredVfxUpdate();

#endif //WITH_EDITOR
};

