// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyComponent.h"

#include "TrueSkyPluginPrivatePCH.h"

UTrueSkyComponent::UTrueSkyComponent(const class FObjectInitializer& PCIP):Super(PCIP)
{
}

void UTrueSkyComponent::OnRegister()
{
    Super::OnRegister();
}

void UTrueSkyComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void UTrueSkyComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
}

void UTrueSkyComponent::OnUnregister()
{
    Super::OnUnregister();
}

