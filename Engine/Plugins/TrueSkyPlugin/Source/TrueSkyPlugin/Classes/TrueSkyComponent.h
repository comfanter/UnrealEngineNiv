// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "TrueSkyComponent.generated.h"
UCLASS(ClassGroup=Rendering,MinimalAPI,hidecategories=(Object, ActorComponent))
class UTrueSkyComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UActorComponent interface.
	virtual void OnRegister() override;
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void OnUnregister() override;
	// Begin UActorComponent interface.

protected:
};

