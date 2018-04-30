// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.
#include "TrueSkySettings.h"
#include "TrueSkyPluginPrivatePCH.h"

UTrueSkySettings::UTrueSkySettings(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
	,DefaultCloudSteps(200)
	,MinimumViewSize(256)
	,MinimumViewSizeForTranslucent(400)
	,MaxFramesBetweenViewUpdates(100)
	,RenderOnlyIdentifiedViews(true)
	,WorleyTextureSize(64)
{

}
