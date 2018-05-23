// Copyright 2018 BruceLee, Inc. All Rights Reserved.


#include "PivotToolSettings.h"


UPivotToolSettings::UPivotToolSettings()
	: bDisplayPivotPresetPreview(true)
	, PivotPresetPreviewSize(10.f)
	, PivotPresetPreviewLineThickness(1.f)
	, PivotPresetPreviewColor(FColor::Yellow)
{
	if (!IsRunningCommandlet())
	{
	}
}
