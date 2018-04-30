// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyLightComponent.h"

#include "TrueSkyPluginPrivatePCH.h"

#if WITH_EDITORONLY_DATA
#include "ObjectEditorUtils.h"
#include "ConstructorHelpers.h"
#endif
#include "Engine/SkyLight.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "Misc/ScopeLock.h"
#include "Net/UnrealNetwork.h"
#include "MapErrors.h"
#include "ComponentInstanceDataCache.h"
#include "ShaderCompiler.h"
#include "Engine/TextureCube.h"		// So that UTextureCube* can convert to UTexture* when calling FSceneInterface::UpdateSkyCaptureContents
#include "Engine/TextureRenderTargetCube.h"
#include "SceneManagement.h"
#include "UObject/UObjectIterator.h"
#include <algorithm>

#define LOCTEXT_NAMESPACE "TrueSkyLightComponent"

extern ENGINE_API int32 GReflectionCaptureSize;

TArray<UTrueSkyLightComponent*> UTrueSkyLightComponent::TrueSkyLightComponents;

bool UTrueSkyLightComponent::AllSkylightsValid=false;

UTrueSkyLightComponent::UTrueSkyLightComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet())
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> StaticTexture(TEXT("/Engine/EditorResources/LightIcons/SkyLight"));
		StaticEditorTexture = StaticTexture.Object;
		StaticEditorTextureScale = 1.0f;
		DynamicEditorTexture = StaticTexture.Object;
		DynamicEditorTextureScale = 1.0f;
	}
#endif
	UpdateFrequency=4;
	Blend=0.5f;
	Brightness_DEPRECATED = 1;
	Intensity = 1;
	DiffuseMultiplier=1;
	SpecularMultiplier=1;
	IndirectLightingIntensity = 1.0f;
	SkyDistanceThreshold = 150000;
	Mobility = EComponentMobility::Stationary;
	bLowerHemisphereIsBlack = true;
	bSavedConstructionScriptValuesValid = true;
	bHasEverCaptured = false;
	OcclusionMaxDistance = 1000;
	MinOcclusion = 0;
	OcclusionTint = FColor::Black;
	bIsInitialized=false;
	bDiffuseInitialized=false;
	SourceType =SLS_CapturedScene;// Not really but this will force it to create the ProcessedSkyTexture
}


void UTrueSkyLightComponent::CreateRenderState_Concurrent()
{
	UActorComponent::CreateRenderState_Concurrent();

	bool bHidden = false;
#if WITH_EDITORONLY_DATA
	bHidden = GetOwner() ? GetOwner()->bHiddenEdLevel : false;
#endif // WITH_EDITORONLY_DATA

	if(!ShouldComponentAddToScene())
	{
		bHidden = true;
	}

	if (bAffectsWorld && bVisible && !bHidden)
	{

		// Create the light's scene proxy. ProcessedSkyTexture may be NULL still here, so we can't use CreateSceneProxy()
		SceneProxy = new FSkyLightSceneProxy(this);
		bIsInitialized=false;
		if (SceneProxy&&GetWorld())
		{
			// Add the light to the scene.
			GetWorld()->Scene->SetSkyLight(SceneProxy);
			const int nums=SceneProxy->IrradianceEnvironmentMap.R.NumSIMDVectors*SceneProxy->IrradianceEnvironmentMap.R.NumComponentsPerSIMDVector;
			for(int i=0;i<nums;i++)
			{
				SceneProxy->IrradianceEnvironmentMap.R.V[i]=0.0f;
				SceneProxy->IrradianceEnvironmentMap.G.V[i]=0.0f;
				SceneProxy->IrradianceEnvironmentMap.B.V[i]=0.0f;
			}
			// This should force the creation of ProcessedSkyTexture...
			if (!ProcessedSkyTexture)
				SetCaptureIsDirty();
		}
	}
}

void UTrueSkyLightComponent::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject)&&!HasAnyFlags(RF_ArchetypeObject))
	{
		// Enqueue an update by default, so that newly placed components will get an update
		// PostLoad will undo this for components loaded from disk
		TrueSkyLightComponents.AddUnique(this);
	}
	AllSkylightsValid=false;
	// Skip over USkyLightComponent::PostInitProperties to avoid putting this skylight in the update list
	// as that would trash the calculated values.
	//ULightComponentBase::PostInitProperties();
	// BUT: That means the proxy won't get created. We need to do at least ONE capture to make sure it will be.
	UActorComponent::PostInitProperties();
}

void UTrueSkyLightComponent::PostLoad()
{
	UActorComponent::PostLoad();

	SanitizeCubemapSize();

	// All components are queued for update on creation by default, remove if needed
	if (!bVisible || HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		FScopeLock Lock(&SkyCapturesToUpdateLock);
		SkyCapturesToUpdate.Remove(this);
	}

	// All components are queued for update on creation by default, remove if needed
	if (!bVisible || HasAnyFlags(RF_ClassDefaultObject)||HasAnyFlags(RF_ArchetypeObject))
	{
		TrueSkyLightComponents.Remove(this);
	}
}


/** 
* This is called when property is modified by InterpPropertyTracks
*
* @param PropertyThatChanged	Property that changed
*/
void UTrueSkyLightComponent::PostInterpChange(UProperty* PropertyThatChanged)
{
	static FName LightColorName(TEXT("LightColor"));
	static FName IntensityName(TEXT("Intensity"));
	static FName DiffuseMultiplierName(TEXT("DiffuseMultiplier"));
	static FName SpecularMultiplierName(TEXT("SpecularMultiplier"));
	static FName IndirectLightingIntensityName(TEXT("IndirectLightingIntensity"));

	FName PropertyName = PropertyThatChanged->GetFName();
	if (PropertyName == LightColorName
		|| PropertyName == IntensityName
		|| PropertyName == IndirectLightingIntensityName
		|| PropertyName == DiffuseMultiplierName
		|| PropertyName == SpecularMultiplierName)
	{
		UpdateLimitedRenderingStateFast();
	}
	else
	{
		UActorComponent::PostInterpChange(PropertyThatChanged);
	}
}

void UTrueSkyLightComponent::DestroyRenderState_Concurrent()
{
	// Skip Super.
	UActorComponent::DestroyRenderState_Concurrent();

	if (SceneProxy)
	{
		if(GetWorld())
		{
		GetWorld()->Scene->DisableSkyLight(SceneProxy);
		}

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDestroySkyLightCommand,
			FSkyLightSceneProxy*,LightSceneProxy,SceneProxy,
		{
			delete LightSceneProxy;
		});

		SceneProxy = NULL;
	}
}

#if WITH_EDITOR
void UTrueSkyLightComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Pass the variables to the parent
	// bLowerHemisphereIsBlack = LowerHemisphereIsBlack;
	// LowerHemisphereColor = SkylightLowerHemisphereColor;
	SetInitialized(false);
	// Skip over USkyLightComponent::PostEditChangeProperty to avoid trashing values.
	ULightComponentBase::PostEditChangeProperty(PropertyChangedEvent);
}

bool UTrueSkyLightComponent::CanEditChange(const UProperty* InProperty) const
{
	if (InProperty)
	{
		FString PropertyName = InProperty->GetName();

		if (FCString::Strcmp(*PropertyName, TEXT("Cubemap")) == 0
			|| FCString::Strcmp(*PropertyName, TEXT("SourceCubemapAngle")) == 0)
		{
			return SourceType == SLS_SpecifiedCubemap;
		}

		if (FCString::Strcmp(*PropertyName, TEXT("Contrast")) == 0
			|| FCString::Strcmp(*PropertyName, TEXT("OcclusionMaxDistance")) == 0
			|| FCString::Strcmp(*PropertyName, TEXT("MinOcclusion")) == 0
			|| FCString::Strcmp(*PropertyName, TEXT("OcclusionTint")) == 0)
		{
			static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.GenerateMeshDistanceFields"));
			return Mobility == EComponentMobility::Movable && CastShadows && CVar->GetValueOnGameThread() != 0;
		}
	}

	return UActorComponent::CanEditChange(InProperty);
}

void UTrueSkyLightComponent::CheckForErrors()
{
	AActor* Owner = GetOwner();

	if (Owner && bVisible && bAffectsWorld)
	{
		UWorld* ThisWorld = Owner->GetWorld();
		bool bMultipleFound = false;

		if (ThisWorld)
		{
			for (TObjectIterator<UTrueSkyLightComponent> ComponentIt; ComponentIt; ++ComponentIt)
			{
				UTrueSkyLightComponent* Component = *ComponentIt;

				if (Component != this 
					&& !Component->IsPendingKill()
					&& Component->bVisible
					&& Component->bAffectsWorld
					&& Component->GetOwner() 
					&& ThisWorld->ContainsActor(Component->GetOwner())
					&& !Component->GetOwner()->IsPendingKill())
				{
					bMultipleFound = true;
					break;
				}
			}
		}

		if (bMultipleFound)
		{
			FMessageLog("MapCheck").Error()
				->AddToken(FUObjectToken::Create(Owner))
				->AddToken(FTextToken::Create(LOCTEXT( "MapCheck_Message_MultipleSkyLights", "Multiple sky lights are active, only one can be enabled per world." )))
				->AddToken(FMapErrorToken::Create(FMapErrors::MultipleSkyLights));
		}
	}
}

#endif // WITH_EDITOR

void UTrueSkyLightComponent::BeginDestroy()
{
	USkyLightComponent::BeginDestroy();
	TrueSkyLightComponents.Remove(this);
}

bool UTrueSkyLightComponent::IsReadyForFinishDestroy()
{
	return UActorComponent::IsReadyForFinishDestroy() && ReleaseResourcesFence.IsFenceComplete();
}


FActorComponentInstanceData* UTrueSkyLightComponent::GetComponentInstanceData() const
{
	return USkyLightComponent::GetComponentInstanceData();
}

void UTrueSkyLightComponent::RecaptureSky()
{
// don't	SetCaptureIsDirty();
}

void UTrueSkyLightComponent::SetHasUpdated()
{
	bHasEverCaptured = true;

}

void UTrueSkyLightComponent::UpdateDiffuse(float *shValues)
{
	if(!SceneProxy)
		return;
	// There are two things to do here. First we must fill in the output texture.
	// second, we must put numbers in the spherical harmonics vector.
	static float m=0.05f;
	float mix=m;
	static int maxnum=9;
	const int nums=SceneProxy->IrradianceEnvironmentMap.R.MaxSHBasis;
	int NumTotalFloats=SceneProxy->IrradianceEnvironmentMap.R.NumSIMDVectors*SceneProxy->IrradianceEnvironmentMap.R.NumComponentsPerSIMDVector;
	float multiplier=DiffuseMultiplier/std::max(0.01f,SpecularMultiplier);
	if(!bDiffuseInitialized)
	{
		for(int j=0;j<NumTotalFloats;j++)
		{
			SceneProxy->IrradianceEnvironmentMap.R.V[j]=0.0f;
			SceneProxy->IrradianceEnvironmentMap.G.V[j]=0.0f;
			SceneProxy->IrradianceEnvironmentMap.B.V[j]=0.0f;
		}
		mix=1.0f;
	}
	for(int j=0;j<nums;j++)
	{
		SceneProxy->IrradianceEnvironmentMap.R.V[j]*=(1.0f-mix);
		SceneProxy->IrradianceEnvironmentMap.G.V[j]*=(1.0f-mix);
		SceneProxy->IrradianceEnvironmentMap.B.V[j]*=(1.0f-mix);
		SceneProxy->IrradianceEnvironmentMap.R.V[j]+=mix*shValues[j*3]*multiplier;
		SceneProxy->IrradianceEnvironmentMap.G.V[j]+=mix*shValues[j*3+1]*multiplier;
		SceneProxy->IrradianceEnvironmentMap.B.V[j]+=mix*shValues[j*3+2]*multiplier;
	}
	for(int j=maxnum;j<NumTotalFloats;j++)
	{
		SceneProxy->IrradianceEnvironmentMap.R.V[j]=0.0f;
		SceneProxy->IrradianceEnvironmentMap.G.V[j]=0.0f;
		SceneProxy->IrradianceEnvironmentMap.B.V[j]=0.0f;
	}
	AverageBrightness=(0.3f*SceneProxy->IrradianceEnvironmentMap.R.V[0]+0.4f*SceneProxy->IrradianceEnvironmentMap.G.V[0]+0.3f*SceneProxy->IrradianceEnvironmentMap.B.V[0])/3.141f;
	SceneProxy->AverageBrightness=AverageBrightness;
	SetInitialized(true);
	bDiffuseInitialized=true;
}

FTexture *UTrueSkyLightComponent::GetCubemapTexture()
{
	return ProcessedSkyTexture;
}

void UTrueSkyLightComponent::ResetTrueSkyLight()
{
	bIsInitialized=false;
	bDiffuseInitialized=false;
}
#undef LOCTEXT_NAMESPACE