// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USH_WaterfallPathComponent;
class USH_WaterfallSettingsComponent;
class ASH_Waterfall2;

/* Base class for generation structs with shared processing varibales/functions. */
struct SHADERSOURCE_WATERFALL2RUNTIME_API FSH_WaterfallBuilder
{
public:
	FSH_WaterfallBuilder() {}
	FSH_WaterfallBuilder(ASH_Waterfall2* _ParentWaterfall);
	virtual ~FSH_WaterfallBuilder() {}

#if WITH_EDITOR
	//Note: This can return nullptr so it should be checked
	ASH_Waterfall2* GetParentWaterfall() { return ParentWaterfall; }
	//Note: This can return nullptr so it should be checked
	USH_WaterfallSettingsComponent* GetWaterfallSettings();

private:
	ASH_Waterfall2* ParentWaterfall = nullptr;
	USH_WaterfallSettingsComponent* SettingsComponent = nullptr;

public:
	virtual void Tick(float DeltaTime) {}

	bool IsProcessing() { return bIsProcessing; }

protected:
	TArray<USH_WaterfallPathComponent*> PathsToProcess = {};
	TArray<USH_WaterfallPathComponent*> PathsProcessing = {};
	int32 NumPathsProcessed = 0;

	FSimpleDelegate OnEndProcessing;
	virtual void StartProcessing();
	virtual void ProcessingFinished();
	void SetIsProcessing(bool bNewIsProcessing) { bIsProcessing = bNewIsProcessing; }

private:
	bool bIsProcessing = false;
#endif
};
