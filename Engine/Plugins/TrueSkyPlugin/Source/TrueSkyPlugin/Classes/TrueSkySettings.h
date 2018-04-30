// Copyright 2007-2017 Simul Software Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "TrueSkySettings.generated.h"


UCLASS(config=Engine,defaultconfig)
class TRUESKYPLUGIN_API UTrueSkySettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	
	/** The number of raytracing steps to take when rendering clouds, larger values are slower to render.*/
	UPROPERTY(config, EditAnywhere, Category=Rendering)
	int32 DefaultCloudSteps;
	
	/** The smallest size in pixels for a view to render trueSKY.*/
	UPROPERTY(config, EditAnywhere, Category=Rendering)
	int32 MinimumViewSize;
	
	/** The smallest size in pixels for a view to include translucent elements (rain, snow etc).*/
	UPROPERTY(config, EditAnywhere, Category=Rendering)
	int32 MinimumViewSizeForTranslucent;

	/** The maximum number of frames between updates of any view with trueSKY. If a view goes longer than this without updating, it will be freed to save memory.
	If set to zero, views will never be freed until the end of a level.*/
	UPROPERTY(config, EditAnywhere, Category = Rendering, meta = (ClampMin = "0", ClampMax = "1000"))
	int32 MaxFramesBetweenViewUpdates;

	/** If true, trueSKY will only render views that have a unique id assigned by UE. This excludes captures.*/
	UPROPERTY(config, EditAnywhere, Category = Rendering)
	bool RenderOnlyIdentifiedViews;

	/** Size of the 3D cell-noise texture used to generate clouds. Larger values use more GPU memory.*/
	UPROPERTY(config, EditAnywhere, Category = "Generation", meta = (ClampMin = "32", ClampMax = "256"))
	int32 WorleyTextureSize;
};
