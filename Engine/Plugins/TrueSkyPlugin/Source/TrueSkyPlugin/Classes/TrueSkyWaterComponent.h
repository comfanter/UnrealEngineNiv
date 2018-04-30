// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "TrueSkyWaterComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = Water, HideCategories = (Trigger, Activation, "Components|Activation", Physics, Light), meta = (BlueprintSpawnableComponent))
class TRUESKYPLUGIN_API UTrueSkyWaterComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	static TArray<UTrueSkyWaterComponent*> TrueSkyWaterComponents;

	void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;
};