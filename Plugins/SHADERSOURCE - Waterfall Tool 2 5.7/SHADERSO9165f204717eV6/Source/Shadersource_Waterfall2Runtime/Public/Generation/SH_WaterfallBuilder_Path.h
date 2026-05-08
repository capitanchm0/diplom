// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Generation/SH_WaterfallBuilder.h"

/* Builder for waterfall paths (creates the splines by simulating a fluid particle). */
struct SHADERSOURCE_WATERFALL2RUNTIME_API FSH_WaterfallBuilder_Path : public FSH_WaterfallBuilder
{
	friend class ASH_Waterfall2;

public:
	FSH_WaterfallBuilder_Path() {}
	FSH_WaterfallBuilder_Path(ASH_Waterfall2* _ParentWaterfall)
		: FSH_WaterfallBuilder(_ParentWaterfall)
	{

	}

#if WITH_EDITOR
	//Simulation (Create Paths)
public:
	virtual void StartSimulate(FSimpleDelegate OnProcessingFinished = FSimpleDelegate());
	virtual void CancelSimulate();
	void DeleteAllPaths(bool bDeleteMeshesToo = true);

protected:
	TArray<USH_WaterfallPathComponent*> ProcessedPaths = {};

	virtual void SanitiseProcessedPaths();

	virtual void Tick(float DeltaTime) override;
	virtual void EndSimulate();

	virtual void CreatePath(int32 Index);
	bool RegisterPath(USH_WaterfallPathComponent* PathToRegister);

	virtual void ProcessingFinished() override;
#endif
};
