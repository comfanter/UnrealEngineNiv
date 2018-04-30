// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyWaterActor.h"

#include "TrueSkyPluginPrivatePCH.h"

#include "WaterActorCrossThreadProperties.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"

#include <map>
#include <atomic>

static std::atomic<int> IDCount(0);

static std::map<UWorld*, ATrueSkyWater *> worldCurrentActiveWaterActors;

ATrueSkyWater::ATrueSkyWater(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	ID(0)
{
	WaterComponent = CreateDefaultSubobject<UTrueSkyWaterComponent>(TEXT("WaterComponent"));
	RootComponent = WaterComponent;

	SetActorTickEnabled(true);
}

ATrueSkyWater::~ATrueSkyWater()
{
	ITrueSkyPlugin::Get().RemoveBoundedWaterObject(ID);
	//RemoveWaterActorCrossThreadProperties(ID);
/** Returns LightComponent subobject **/
	for (auto i = worldCurrentActiveWaterActors.begin(); i != worldCurrentActiveWaterActors.end(); i++)
	{
		if (i->second == this)
		{
			worldCurrentActiveWaterActors.erase(i);
			break;
		}
	}
}

void ATrueSkyWater::PostInitProperties()
{
	Super::PostInitProperties();

	WaterActorCrossThreadProperties* A = new WaterActorCrossThreadProperties;
	
	if (!initialised)
	{
		render = false;
		initialised = true;
		beaufortScale = A->BeaufortScale;
		windDirection = A->WindDirection;
		windDependency = A->WindDependency;
		scattering = A->Scattering;
		absorption = A->Absorption;
		dimension = A->Dimension;
		salinity = A->Salinity;
		temperature = A->Temperature;
		advancedWaterOptions = A->AdvancedWaterOptions;
		windSpeed = A->WindSpeed;
		waveAmplitude = A->WaveAmplitude;
		choppyScale = A->ChoppyScale;
	}
	IDCount++;
	ID = IDCount;
	A->ID = IDCount;

	AddWaterActorCrossThreadProperties(A);

	TransferProperties();

	if (RootComponent)
		RootComponent->UpdateComponentToWorld();

	A->Location = GetActorLocation();
	ITrueSkyPlugin::EnsureLoaded();
	// TODO: don't create for reference object
	ITrueSkyPlugin::Get().CreateBoundedWaterObject(ID, (float*)&A->Dimension, (float*)&A->Location);
}

void ATrueSkyWater::PostLoad()
{
	Super::PostLoad();
	if (RootComponent)
		RootComponent->UpdateComponentToWorld();
	//ITrueSkyPlugin::Get().CreateBoundedWaterObject(ID, (float*)&A->Dimension, (float*)&A->Location);
	//TransferProperties();
}

void ATrueSkyWater::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (RootComponent)
		RootComponent->UpdateComponentToWorld();
	TransferProperties();
}

void ATrueSkyWater::Destroyed()
{
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	if (!A)
		return;
	ITrueSkyPlugin::Get().RemoveBoundedWaterObject(ID);
	A->Destroyed = true;
	AActor::Destroyed();
}

void ATrueSkyWater::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//ITrueSkyPlugin::Get().TriggerAction("Reset");
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	if (A)
		A->Reset = true;
}

void ATrueSkyWater::TransferProperties()
{
	if (!ITrueSkyPlugin::IsAvailable())
		return;
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	if (RootComponent)
		RootComponent->UpdateComponentToWorld();
	if (A!=NULL)
	{
		A->Render = render;
		A->Destroyed = false;
		A->Reset = false;
		A->BeaufortScale = beaufortScale;
		A->WindDirection = windDirection;
		A->WindDependency = windDependency;
		A->Scattering = scattering;
		A->Absorption = absorption;
		A->Location = GetActorLocation();
		A->Dimension = dimension;
		A->Salinity = salinity;
		A->Temperature = temperature;
		A->AdvancedWaterOptions = advancedWaterOptions;
		A->WindSpeed = windSpeed;
		A->WaveAmplitude = waveAmplitude;
		A->ChoppyScale = choppyScale;
		if (!A->BoundedObjectCreated)
		{
			ITrueSkyPlugin::Get().CreateBoundedWaterObject(ID, (float*)&A->Dimension, (float*)&A->Location);
		}
	}
}

FBox ATrueSkyWater::GetComponentsBoundingBox(bool bNonColliding) const
{
	FBox Box(EForceInit::ForceInit);

	for (const UActorComponent* ActorComponent : GetComponents())
	{
		const UBoxComponent* PrimComp = Cast<const UBoxComponent>(ActorComponent);
		if (PrimComp)
		{
			// Only use collidable components to find collision bounding box.
			if (PrimComp->IsRegistered() && (bNonColliding || PrimComp->IsCollisionEnabled()))
			{
				Box += PrimComp->Bounds.GetBox();
			}
		}
	}
	return Box;
}


#if WITH_EDITOR
void ATrueSkyWater::EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	Super::EditorApplyTranslation(DeltaTranslation, bAltDown, bShiftDown, bCtrlDown);
	TransferProperties();
}
void ATrueSkyWater::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (RootComponent)
		RootComponent->UpdateComponentToWorld();
	TransferProperties();
}
bool ATrueSkyWater::CanEditChange(const UProperty* InProperty) const
{
	if (InProperty)
	{
		if (!ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
			return false;

		FString PropertyName = InProperty->GetName();

		if (!advancedWaterOptions)
		{
			if (PropertyName == "WindSpeed"
				|| PropertyName == "WaveAmplitude"
				|| PropertyName == "ChoppyScale")
				return false;
		}
	}
	return true;
}
#endif

/** ReturnsWaterComponent subobject **/
UTrueSkyWaterComponent* ATrueSkyWater::GetWaterComponent() const { return WaterComponent; }