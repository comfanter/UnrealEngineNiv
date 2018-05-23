// Copyright 2018 BruceLee, Inc. All Rights Reserved.

#pragma once

#include "CoreUObject.h"
#include "PivotToolSettings.generated.h"

UCLASS(config = Editor, defaultconfig)
class UPivotToolSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category = PivotPreset)
		bool bDisplayPivotPresetPreview;

	UPROPERTY(config, EditAnywhere, Category = PivotPreset)
		float PivotPresetPreviewSize;

	UPROPERTY(config, EditAnywhere, Category = PivotPreset)
		float PivotPresetPreviewLineThickness;

	UPROPERTY(config, EditAnywhere, Category = PivotPreset)
		FColor PivotPresetPreviewColor;

public:
	UPivotToolSettings();
};
