// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkySequenceActor.h"

#include "TrueSkyPluginPrivatePCH.h"

#include "Classes/Engine/Engine.h"

#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Light.h"
#include "Engine/PointLight.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetCube.h"
#include "GameFramework/Actor.h"
#include "ActorCrossThreadProperties.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Logging/LogMacros.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DirectionalLightComponent.h"
#include "TrueSkyLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "TimerManager.h"
#include "TrueSkyComponent.h"
#include <atomic>
#include <map>
#include <algorithm>

static std::atomic<int> actors(0);

static const float PI_F=3.1415926536f;

#ifndef UE_LOG4_ONCE
#define UE_LOG4_ONCE(tsa_a,tsa_b,tsa_c,d) {static bool done=false; if(!done) {UE_LOG(tsa_a,tsa_b,tsa_c,d);done=true;}}
#endif

std::map<UWorld*, ATrueSkySequenceActor *> worldCurrentActiveSequenceActors;
using namespace simul::unreal;
static float m1=1.0f,m2=-1.0f,m3=0,m4=0;
static float c1=0.0f,c2=90.0f,c3=0.0f;
static float yo=180.0;
static float tsa_a=-1.0f,tsa_b=-1.0f,tsa_c=-1.0f;
static FVector tsa_dir, tsa_u;
static FTransform worldToSky;

float ATrueSkySequenceActor::Time=0.0f;

bool ATrueSkySequenceActor::bSetTime=false;
// These will get overwritten by the proper logic, but likely will result in the same values.
int32 ATrueSkySequenceActor::TSSkyLayerPropertiesFloatIdx = 0;
int32 ATrueSkySequenceActor::TSSkyKeyframePropertiesFloatIdx = 1;
int32 ATrueSkySequenceActor::TSCloudLayerPropertiesFloatIdx = 2;
int32 ATrueSkySequenceActor::TSCloudKeyframePropertiesFloatIdx = 3;
int32 ATrueSkySequenceActor::TSSkyLayerPropertiesIntIdx = 0;
int32 ATrueSkySequenceActor::TSSkyKeyframePropertiesIntIdx = 1;
int32 ATrueSkySequenceActor::TSCloudLayerPropertiesIntIdx = 2;	
int32 ATrueSkySequenceActor::TSCloudKeyframePropertiesIntIdx = 3;

int32 ATrueSkySequenceActor::TSPropertiesIntIdx=4;
int32 ATrueSkySequenceActor::TSPropertiesFloatIdx=4;
int32 ATrueSkySequenceActor::TSPositionalPropertiesFloatIdx=5;
int32 ATrueSkySequenceActor::TSPropertiesVectorIdx=0;

TArray< ATrueSkySequenceActor::TrueSkyPropertyIntMap > ATrueSkySequenceActor::TrueSkyPropertyIntMaps;
TArray< ATrueSkySequenceActor::TrueSkyPropertyFloatMap > ATrueSkySequenceActor::TrueSkyPropertyFloatMaps;
TArray< ATrueSkySequenceActor::TrueSkyPropertyVectorMap > ATrueSkySequenceActor::TrueSkyPropertyVectorMaps;

const FString ATrueSkySequenceActor::kDefaultMoon = "Texture2D'/TrueSkyPlugin/Moon.Moon'";
const FString ATrueSkySequenceActor::kDefaultMilkyWay = "Texture2D'/TrueSkyPlugin/MilkyWay.MilkyWay'";
const FString ATrueSkySequenceActor::kDefaultTrueSkyInscatterRT = "TextureRenderTarget2D'/TrueSkyPlugin/trueSKYInscatterRT.trueSKYInscatterRT'";
const FString ATrueSkySequenceActor::kDefaultTrueSkyLossRT = "TextureRenderTarget2D'/TrueSkyPlugin/trueSKYLossRT.trueSKYLossRT'";
const FString ATrueSkySequenceActor::kDefaultCloudShadowRT = "TextureRenderTarget2D'/TrueSkyPlugin/cloudShadowRT.cloudShadowRT'";
const FString ATrueSkySequenceActor::kDefaultCloudVisibilityRT = "TextureRenderTarget2D'/TrueSkyPlugin/cloudVisibilityRT.cloudVisibilityRT'";
simul::QueryMap truesky_queries;

static FTextureRHIRef GetTextureRHIRef(UTexture *t)
{
	if (!t)
		return nullptr;
	if (!t->Resource)
		return nullptr;
	return t->Resource->TextureRHI;
}
static FTextureRHIRef GetTextureRHIRef(UTextureRenderTargetCube *t)
{
	if (!t)
		return nullptr;
	if (!t->Resource)
		return nullptr;
	return t->Resource->TextureRHI;
}

ATrueSkySequenceActor::ATrueSkySequenceActor(const class FObjectInitializer& PCIP)
	: Super(PCIP)
	,InterpolationMode(EInterpolationMode::FixedNumber)
	,InterpolationTimeSeconds(10.0f)
	,InterpolationTimeHours(5.0f)
	,VelocityStreaks(true)
	,SimulationTime(false)
	,RainNearThreshold(30.0f)
	,Light(nullptr)
	,DriveLightDirection(true)
	,SunMultiplier(1.0f)
	,MoonMultiplier(1.0f)
	,RenderWater(false)
	,EnableBoundlessOcean(false)
	,OceanBeaufortScale(0.0f)
	,OceanWindDirection(0.0f)
	,OceanWindDependency(0.0f)
	,OceanScattering(0.0f,0.0f,0.0f)
	,OceanAbsorption(0.0f,0.0f,0.0f)
	,AdvancedWaterOptions(false)
	,OceanWindSpeed(0.0f)
	,OceanWaveAmplitude(0.0f)
	,OceanChoppyScale(0.0f)
	,OceanFoamHeight(0.0f)
	,OceanFoamChurn(0.0f)
	,FixedDeltaTime(-1.0f)
	,MaxSunRadiance( 150000.0f )
	,AdjustSunRadius(true)
	,ActiveInEditor(true)
	,ActiveSequence(nullptr)
	,MoonTexture(nullptr)
	,CosmicBackgroundTexture(nullptr)
	,RainCubemap(nullptr)
	,RainMaskSceneCapture2D(nullptr)
	,RainMaskWidth(1000.0f)
	,RainDepthExtent(2500.0f)
	,InscatterRT(nullptr)
	,LossRT(nullptr)
	,CloudShadowRT(nullptr)
	,CloudVisibilityRT(nullptr)
	,IntegrationScheme(EIntegrationScheme::Grid)
	,Brightness(1.0f)
	,MetresPerUnit(0.01f)
	,SimpleCloudShadowing(0.5f)
	,SimpleCloudShadowSharpness(0.01f)
	,CloudThresholdDistanceKm(0.0f)
	,DownscaleFactor(2)
	,MaximumResolution(256)
	,DepthSamplingPixelRange(1.5f)
	,Amortization(3)
	,AtmosphericsAmortization(4)
	,DepthBlending(true)
	,MinimumStarPixelSize(1.5f)
	,ShareBuffersForVR(true)
	,Visible(true)
	,TrueSkyMaterialParameterCollection(nullptr)
	,latest_thunderbolt_id(-1)
	,bPlaying(false)
	,bSimulVersion4_1(false)
	,bInterpolationFixed( true )
	,bInterpolationGameTime( false )
	,bInterpolationRealTime( false )
	,initializeDefaults(false)
	,wetness(0.0f)
	,CaptureTargetCube(nullptr)
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	PrimaryActorTick.bTickEvenWhenPaused	=true;
	PrimaryActorTick.bCanEverTick			=true;
	PrimaryActorTick.bStartWithTickEnabled	=true;
	SetTickGroup( TG_DuringPhysics);
	SetActorTickEnabled(true);
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	++actors;
	// we must have TWO actors in order for one to be in tsa_a scene, because there's also tsa_a DEFAULT object!
	if(A&&actors>=2)
		A->Destroyed=false;
	latest_thunderbolt_id=0;
	GodraysGrid[0]=64;
	GodraysGrid[1]=32;
	GodraysGrid[2]=32;

	// Setup the asset references for the default textures/RTs.
	MoonTextureAssetRef = kDefaultMoon;
	MilkyWayAssetRef = kDefaultMilkyWay;
	TrueSkyInscatterRTAssetRef = kDefaultTrueSkyInscatterRT;
	TrueSkyLossRTAssetRef = kDefaultTrueSkyLossRT;
	CloudShadowRTAssetRef = kDefaultCloudShadowRT;
	CloudVisibilityRTAssetRef = kDefaultCloudVisibilityRT;
}

ATrueSkySequenceActor::~ATrueSkySequenceActor()
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(!A)
		return;
	actors--;
	if(A&&actors<=1)
		A->Destroyed=true;
	for( auto i = worldCurrentActiveSequenceActors.begin( ); i != worldCurrentActiveSequenceActors.end( ); i++ )
	{
		if(i->second==this)
		{
			worldCurrentActiveSequenceActors.erase( i );
			break;
		}
	}
	if( !worldCurrentActiveSequenceActors.size( ) )
	{
		A->Visible=false;
		A->activeSequence=nullptr;
	}
}

float ATrueSkySequenceActor::GetReferenceSpectralRadiance()
{
	float r	=GetTrueSkyPlugin()->GetRenderFloat("ReferenceSpectralRadiance");
	return r;
}

int32 ATrueSkySequenceActor::TriggerAction(FString name)
{
	int32 r	=GetTrueSkyPlugin()->TriggerAction(name);
	return r;
}

UFUNCTION( BlueprintCallable,Category = TrueSky)
int32 ATrueSkySequenceActor::SpawnLightning(FVector pos1,FVector pos2,float magnitude,FVector colour)
{
	return GetTrueSkyPlugin()->SpawnLightning(pos1,pos2,magnitude,colour);
}

int32 ATrueSkySequenceActor::GetLightning(FVector &start,FVector &end,float &magnitude,FVector &colour)
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	int i=0;
	{
		simul::LightningBolt *L=&(A->lightningBolts[i]);
		if(L&&L->id!=0)
		{
			start=FVector(L->pos[0],L->pos[1],L->pos[2]);
			end=FVector(L->endpos[0],L->endpos[1],L->endpos[2]);
			magnitude=L->brightness;
			colour=FVector(L->colour[0],L->colour[1],L->colour[2]);
			return L->id;
		}
		else
		{
			magnitude=0.0f;
	}
	}
	return 0;
}

void ATrueSkySequenceActor::PostRegisterAllComponents()
{
	UWorld *world = GetWorld();
	// If any other Sequence Actor is already ActiveInEditor, make this new one Inactive.
	if (ActiveInEditor&&world!=nullptr&&world->WorldType == EWorldType::Editor)
	{
		bool in_list = false;
		for (TActorIterator<ATrueSkySequenceActor> Itr(world); Itr; ++Itr)
		{
			ATrueSkySequenceActor* SequenceActor = *Itr;
			if (SequenceActor == this)
			{
				in_list = true;
				continue;
			}
			if (SequenceActor->ActiveInEditor)
			{
				ActiveInEditor = false;
				break;
			}
		}
		if(in_list)
			worldCurrentActiveSequenceActors[world] = this;
	}
}


void ATrueSkySequenceActor::PostInitProperties()
{
    Super::PostInitProperties();
	// We need to ensure this first, or we may get bad data in the transform:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();

	bInterpolationFixed = ( InterpolationMode == EInterpolationMode::FixedNumber );
	bInterpolationGameTime = ( InterpolationMode == EInterpolationMode::GameTime );
	bInterpolationRealTime = ( InterpolationMode == EInterpolationMode::RealTime );

	// NOTE: Although this is called "PostInitProperties", it won't actually have the initial properties in the Actor instance.
	// those are set... somewhere... later.
	// So we MUST NOT call TransferProperties here, or else we will get GPU memory etc allocated with the wrong values.
	//TransferProperties();
	UpdateVolumes();// no use
}

void ATrueSkySequenceActor::UpdateVolumes()
{
	if (!IsInGameThread() && !ITrueSkyPlugin::IsAvailable())
		return;
	if (!ITrueSkyPlugin::IsAvailable())
		return;

	//TArray< USceneComponent * > comps;
	TArray<UActorComponent *> comps;
	this->GetComponents(comps);

	GetTrueSkyPlugin()->ClearCloudVolumes();
	for(int i=0;i<comps.Num();i++)
	{
		UActorComponent *a=comps[i];
		if(!a)
			continue;
	}
}

void ATrueSkySequenceActor::SetDefaultTextures()
{
#define DEFAULT_PROPERTY(type,propname,filename) \
	if(propname==nullptr)\
	{\
		propname = LoadObject< type >( nullptr, *( filename.ToString( ) ) );\
	}

	DEFAULT_PROPERTY( UTexture2D, MoonTexture, MoonTextureAssetRef )
	DEFAULT_PROPERTY( UTexture2D, CosmicBackgroundTexture, MilkyWayAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, InscatterRT, TrueSkyInscatterRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, LossRT, TrueSkyLossRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, CloudShadowRT, CloudShadowRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, CloudVisibilityRT, CloudVisibilityRTAssetRef )

#undef DEFAULT_PROPERTY
}

void ATrueSkySequenceActor::PostLoad()
{
    Super::PostLoad();
	truesky_queries.Reset();
	// Now, apparently, we have good data.
	// We need to ensure this first, or we may get bad data in the transform:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	if(initializeDefaults)
	{
		SetDefaultTextures();
		initializeDefaults=false;
	}
	latest_thunderbolt_id=0;
	bSetTime=false;
	TransferProperties();
}

void ATrueSkySequenceActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();
	// We need to ensure this first, or we may get bad data in the transform:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	latest_thunderbolt_id=0;
	TransferProperties();

	InitializeTrueSkyPropertyMap( );
}

void ATrueSkySequenceActor::Destroyed()
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(!A)
		return;
	A->Destroyed=true;
	AActor::Destroyed();
}

void ATrueSkySequenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	latest_thunderbolt_id=0;
	//ITrueSkyPlugin::Get().TriggerAction("Reset");
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(!A)
		A->Reset=true;
}

void ATrueSkySequenceActor::FreeViews()
{
	GetTrueSkyPlugin()->TriggerAction("FreeViews");
}

FString ATrueSkySequenceActor::GetProfilingText(int32 c,int32 g)
{
	GetTrueSkyPlugin()->SetRenderInt("maxCpuProfileLevel",c);
	GetTrueSkyPlugin()->SetRenderInt("maxGpuProfileLevel",g);
	return GetTrueSkyPlugin()->GetRenderString("profiling");
}

FString ATrueSkySequenceActor::GetText(FString str)
{
	return GetTrueSkyPlugin()->GetRenderString(str);
}

void ATrueSkySequenceActor::SetTime( float value )
{
	Time=value;
	bSetTime=true;
	// Force the actor to transfer properties:
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(A)
		A->Playing=true;
}

float ATrueSkySequenceActor::GetTime(  )
{
	return Time;
}

void ATrueSkySequenceActor::SetInteger( ETrueSkyPropertiesInt Property, int32 Value )
{
	if(TrueSkyPropertyIntMaps.Num())
	{
		auto p=TrueSkyPropertyIntMaps[TSPropertiesIntIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			if(A&&A->SetInts.Num()<100)
			{
				TPair<int64,int32> t;
				t.Key=p->TrueSkyEnum;
				t.Value=Value;
				A->CriticalSection.Lock();
				A->SetInts.Add(t);
				A->CriticalSection.Unlock();
			}
		}
	}
	SetInt( TrueSkyPropertyIntMaps[TSPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ),Value);
}

int32 ATrueSkySequenceActor::GetInteger( ETrueSkyPropertiesInt Property )
{
	return( GetInt( TrueSkyPropertyIntMaps[TSPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

void ATrueSkySequenceActor::GetIntegerRange( ETrueSkyPropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut )
{
	TrueSkyPropertyIntData* data = TrueSkyPropertyIntMaps[TSPropertiesIntIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::SetFloatProperty( ETrueSkyPropertiesFloat Property, float Value )
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
		auto p=TrueSkyPropertyFloatMaps[TSPropertiesFloatIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			if(A&&A->SetFloats.Num()<100)
			{
				TPair<int64,float> t;
				t.Key=p->TrueSkyEnum;
				t.Value=Value;
				A->CriticalSection.Lock();
				A->SetFloats.Add(t);
				A->CriticalSection.Unlock();
			}
		}
	}
}

float ATrueSkySequenceActor::GetFloatProperty( ETrueSkyPropertiesFloat Property )
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
	return( GetFloat( TrueSkyPropertyFloatMaps[TSPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}
	return 0.0f;
}

void ATrueSkySequenceActor::GetFloatPropertyRange( ETrueSkyPropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut )
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
		TrueSkyPropertyFloatData* data = TrueSkyPropertyFloatMaps[TSPropertiesFloatIdx].Find( ( int64 )Property );
		ValueMinOut = data->ValueMin;
		ValueMaxOut = data->ValueMax;
	}
}

void ATrueSkySequenceActor::SetVectorProperty( ETrueSkyPropertiesVector Property, FVector Value )
{
	if(TrueSkyPropertyVectorMaps.Num())
	{
		auto p=TrueSkyPropertyVectorMaps[TSPropertiesVectorIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			 ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			 if(A&&A->SetVectors.Num()<100)
			 {
				 TPair<int64,vec3> t;
				 t.Key=p->TrueSkyEnum;
				 t.Value.x=Value.X;
				 t.Value.y=Value.Y;
				 t.Value.z=Value.Z;
				 A->CriticalSection.Lock();
				 A->SetVectors.Add(t);
				 A->CriticalSection.Unlock();
			 }
		}
	}
}

FVector ATrueSkySequenceActor::GetVectorProperty( ETrueSkyPropertiesVector Property )
{
	if(TrueSkyPropertyVectorMaps.Num())
	{
		auto p=TrueSkyPropertyVectorMaps[TSPropertiesVectorIdx].Find( ( int64 )Property );
		
		if(p&&p->TrueSkyEnum>0)
		{
			return GetTrueSkyPlugin()->GetVector(p->TrueSkyEnum);
		}
		else
		{
			UE_LOG4_ONCE(TrueSky,Error,TEXT("No such vector property %d"), ( int64 )Property);
		}
	}
	return FVector(0,0,0);
}

float ATrueSkySequenceActor::GetFloatPropertyAtPosition( ETrueSkyPositionalPropertiesFloat Property,FVector pos,int32 queryId)
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
		auto p=TrueSkyPropertyFloatMaps[TSPositionalPropertiesFloatIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			if(!truesky_queries.Contains(queryId))
				truesky_queries.Add(queryId);
			auto &q=truesky_queries[queryId];
			q.uid=queryId;
			q.Pos=pos;
			q.LastFrame=GFrameNumber;
			q.Enum=p->TrueSkyEnum;
			if(q.Valid)
				return q.Float;
		}
		return GetTrueSkyPlugin()->GetRenderFloatAtPosition( p->PropertyName.ToString(),pos);
	}
	return 0.0f;
}

void ATrueSkySequenceActor::CleanQueries()
{
	for(auto q:truesky_queries)
	{
		if(GFrameNumber-q.Value.LastFrame>100)
		{
			truesky_queries.Remove(q.Key);
			break;
		}
	}
}

float ATrueSkySequenceActor::GetFloat(FString name )
{
	if(!name.Compare("time",ESearchCase::IgnoreCase))
		return Time;
	else
		return GetTrueSkyPlugin()->GetRenderFloat(name);
}

float ATrueSkySequenceActor::GetFloatAtPosition(FString name,FVector pos)
{
	return GetTrueSkyPlugin()->GetRenderFloatAtPosition(name,pos);
}

void ATrueSkySequenceActor::SetFloat(FString name, float value )
{
	if(!name.Compare("time",ESearchCase::IgnoreCase))
	{
		bSetTime=true;
		Time=value;
		ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
		if(A)
			A->Playing=true;
	}
	else
		GetTrueSkyPlugin()->SetRenderFloat(name,value);
}

int32 ATrueSkySequenceActor::GetInt(FString name ) 
{
	return GetTrueSkyPlugin()->GetRenderInt(name);
}

void ATrueSkySequenceActor::SetInt(FString name, int32 value )
{
	GetTrueSkyPlugin()->SetRenderInt(name,value);
}

void ATrueSkySequenceActor::SetBool(FString name, bool value )
{
	GetTrueSkyPlugin()->SetRenderBool(name,value);
}

float ATrueSkySequenceActor::GetKeyframeFloat(int32 keyframeUid,FString name ) 
{
	return GetTrueSkyPlugin()->GetKeyframeFloat(keyframeUid,name);
}

void ATrueSkySequenceActor::SetKeyframeFloat(int32 keyframeUid,FString name, float value )
{
	GetTrueSkyPlugin()->SetKeyframeFloat(keyframeUid,name,value);
}

int32 ATrueSkySequenceActor::GetKeyframeInt(int32 keyframeUid,FString name ) 
{
	return GetTrueSkyPlugin()->GetKeyframeInt(keyframeUid,name);
}

void ATrueSkySequenceActor::SetKeyframeInt(int32 keyframeUid,FString name, int32 value )
{
	GetTrueSkyPlugin()->SetKeyframeInt(keyframeUid,name,value);
}

int32 ATrueSkySequenceActor::GetNextModifiableSkyKeyframe() 
{
	return GetTrueSkyPlugin()->GetRenderInt("NextModifiableSkyKeyframe");
}
	
int32 ATrueSkySequenceActor::GetNextModifiableCloudKeyframe(int32 layer) 
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	return GetTrueSkyPlugin()->GetRenderInt("NextModifiableCloudKeyframe",arr);
}

int32 ATrueSkySequenceActor::GetCloudKeyframeByIndex(int32 layer,int32 index)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	arr.Push(FVariant(index));
	return GetTrueSkyPlugin()->GetRenderInt("Clouds:KeyframeByIndex",arr);
}

int32 ATrueSkySequenceActor::GetNextCloudKeyframeAfterTime(int32 layer,float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("Clouds:NextKeyframeAfterTime",arr);
}

int32 ATrueSkySequenceActor::GetPreviousCloudKeyframeBeforeTime(int32 layer,float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("Clouds:PreviousKeyframeBeforeTime",arr);
}
	
int32 ATrueSkySequenceActor::GetInterpolatedCloudKeyframe(int32 layer)
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer)); 
	return GetTrueSkyPlugin()->GetRenderInt("InterpolatedCloudKeyframe", arr);
}

int32 ATrueSkySequenceActor::GetInterpolatedSkyKeyframe()
{ 
	return GetTrueSkyPlugin()->GetRenderInt("InterpolatedSkyKeyframe");
}

int32 ATrueSkySequenceActor::GetSkyKeyframeByIndex(int32 index)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(index));
	return GetTrueSkyPlugin()->GetRenderInt("Sky:KeyframeByIndex", arr);
}

int32 ATrueSkySequenceActor::GetNextSkyKeyframeAfterTime(float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("Sky:NextKeyframeAfterTime",arr);
}

int32 ATrueSkySequenceActor::GetPreviousSkyKeyframeBeforeTime(float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("Sky:PreviousKeyframeBeforeTime",arr);
}

void ATrueSkySequenceActor::SetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property, int32 Value )
{
	SetInt( TrueSkyPropertyIntMaps[TSSkyLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

void ATrueSkySequenceActor::SetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value )
{
	SetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSSkyKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

void ATrueSkySequenceActor::SetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32 Value )
{
	SetInt( TrueSkyPropertyIntMaps[TSCloudLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

void ATrueSkySequenceActor::SetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value )
{
	SetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSCloudKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

int32 ATrueSkySequenceActor::GetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property )
{
	return( GetInt( TrueSkyPropertyIntMaps[TSSkyLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

int32 ATrueSkySequenceActor::GetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID )
{
	return( GetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSSkyKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

int32 ATrueSkySequenceActor::GetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property )
{
	return( GetInt( TrueSkyPropertyIntMaps[TSCloudLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

int32 ATrueSkySequenceActor::GetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID )
{
	return( GetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSCloudKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

void ATrueSkySequenceActor::GetSkyLayerIntRange( ETrueSkySkySequenceLayerPropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut )
{
	TrueSkyPropertyIntData* data = TrueSkyPropertyIntMaps[TSSkyLayerPropertiesIntIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::GetSkyKeyframeIntRange( ETrueSkySkySequenceKeyframePropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut )
{
	TrueSkyPropertyIntData* data = TrueSkyPropertyIntMaps[TSSkyKeyframePropertiesIntIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::GetCloudLayerIntRange( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut )
{
	TrueSkyPropertyIntData* data = TrueSkyPropertyIntMaps[TSCloudLayerPropertiesIntIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::GetCloudKeyframeIntRange( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut )
{
	TrueSkyPropertyIntData* data = TrueSkyPropertyIntMaps[TSCloudKeyframePropertiesIntIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::SetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property, float Value )
{
	SetFloat( TrueSkyPropertyFloatMaps[TSSkyLayerPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

void ATrueSkySequenceActor::SetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value )
{
	SetKeyframeFloat( KeyframeUID, TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

void ATrueSkySequenceActor::SetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property, float Value )
{
	SetFloat( TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

void ATrueSkySequenceActor::SetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value )
{
	SetKeyframeFloat( KeyframeUID, TrueSkyPropertyFloatMaps[TSCloudKeyframePropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}

float ATrueSkySequenceActor::GetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property )
{
	TrueSkyPropertyFloatData *p= TrueSkyPropertyFloatMaps[TSSkyLayerPropertiesFloatIdx].Find( ( int64 )Property );

	if(p->TrueSkyEnum>0)
	{
		return GetTrueSkyPlugin()->GetFloat(p->TrueSkyEnum);
	}
	return( GetFloat(p->PropertyName.ToString( ) ) );
}

float ATrueSkySequenceActor::GetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID )
{
	return( GetKeyframeFloat( KeyframeUID, TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

float ATrueSkySequenceActor::GetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property )
{
	return( GetFloat( TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

float ATrueSkySequenceActor::GetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID )
{
	return( GetKeyframeFloat( KeyframeUID, TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

void ATrueSkySequenceActor::GetSkyLayerFloatRange( ETrueSkySkySequenceLayerPropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut )
{
	TrueSkyPropertyFloatData* data = TrueSkyPropertyFloatMaps[TSSkyLayerPropertiesFloatIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::GetSkyKeyframeFloatRange( ETrueSkySkySequenceKeyframePropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut )
{
	TrueSkyPropertyFloatData* data = TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::GetCloudLayerFloatRange( ETrueSkyCloudSequenceLayerPropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut )
{
	TrueSkyPropertyFloatData* data = TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesFloatIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

void ATrueSkySequenceActor::GetCloudKeyframeFloatRange( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut )
{
	TrueSkyPropertyFloatData* data = TrueSkyPropertyFloatMaps[TSCloudKeyframePropertiesFloatIdx].Find( ( int64 )Property );
	ValueMinOut = data->ValueMin;
	ValueMaxOut = data->ValueMax;
}

FRotator ATrueSkySequenceActor::GetSunRotation() 
{
	float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunAzimuthDegrees");		// Azimuth in compass degrees from north.
	float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunElevationDegrees");
	FRotator sunRotation(c1+m1*elevation,c2+m2*azimuth,c3+m3*elevation+m4*azimuth);
	
	tsa_dir= sunRotation.Vector();// = TrueSkyToUEDirection(sunRotation.Vector());
	tsa_u = tsa_dir;
	ActorCrossThreadProperties *A = NULL;
	A = GetActorCrossThreadProperties();
	if (A)
	{
		worldToSky = A->Transform;
		tsa_u=TrueSkyToUEDirection(A->Transform,tsa_dir);
//		tsa_u = worldToSky.InverseTransformVector(tsa_dir);
	}
	float p	=asin(tsa_c*tsa_u.Z)*180.f/PI_F;
	float y	=atan2(tsa_a*tsa_u.Y,tsa_b*tsa_u.X)*180.f/PI_F;
	return FRotator(p,y,0);
}

FRotator ATrueSkySequenceActor::GetMoonRotation() 
{
	float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonAzimuthDegrees");
	float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonElevationDegrees");
	FRotator moonRotation(c1+m1*elevation,c2+m2*azimuth,c3+m3*elevation+m4*azimuth);
	FVector mdir = moonRotation.Vector();// = TrueSkyToUEDirection(sunRotation.Vector());
	FVector m_u = mdir;
	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
	if (A)
	{
		FTransform worldToSky1 = A->Transform;
		m_u = worldToSky1.InverseTransformVector(tsa_dir);
	}
	float p = asin(tsa_c*m_u.Z)*180.f / PI_F;
	float y = atan2(tsa_a*m_u.Y, tsa_b*m_u.X)*180.f / PI_F;
	return FRotator(p, y, 0);
}

void ATrueSkySequenceActor::SetSunRotation(FRotator r)
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	FVector dir		=UEToTrueSkyDirection(A->Transform,r.Vector());
	float elevation	=-r.Pitch*PI_F/180.0f;			// -ve because we want the angle TO the light, not from it.
	float azimuth	=(r.Yaw+yo)*PI_F/180.0f;		// yaw 0 = east, 90=north
	elevation		=asin(-dir.Z);
	azimuth			=atan2(-dir.X,-dir.Y);
	SetVectorProperty( ETrueSkyPropertiesVector::TSPROPVECTOR_SunDirection, -dir );
}

void ATrueSkySequenceActor::SetMoonRotation(FRotator r)
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	FVector dir=UEToTrueSkyDirection(A->Transform,r.Vector());
	SetVectorProperty( ETrueSkyPropertiesVector::TSPROPVECTOR_MoonDirection, -dir );
}

float ATrueSkySequenceActor::CloudLineTest(int32 queryId,FVector StartPos,FVector EndPos) 
{
	return GetTrueSkyPlugin()->CloudLineTest(queryId,StartPos,EndPos);
}

float ATrueSkySequenceActor::CloudPointTest(int32 queryId,FVector pos) 
{
	return GetTrueSkyPlugin()->GetCloudinessAtPosition(queryId,pos);
}

float ATrueSkySequenceActor::GetCloudShadowAtPosition(int32 queryId,FVector pos) 
{
	ITrueSkyPlugin::VolumeQueryResult res= GetTrueSkyPlugin()->GetStateAtPosition(queryId,pos);
	return (1 - res.direct_light);
}

float ATrueSkySequenceActor::GetRainAtPosition(int32 queryId,FVector pos) 
{
	ITrueSkyPlugin::VolumeQueryResult res= GetTrueSkyPlugin()->GetStateAtPosition(queryId,pos);
	return res.precipitation;
}

FLinearColor ATrueSkySequenceActor::GetSunColor() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	FLinearColor col;
	if(!A)
		return col;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetTrueSkyPlugin()->GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		col.R=res.sunlight[0];
		col.G=res.sunlight[1];
		col.B=res.sunlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		col/=m;
	}
	return col;
}

FLinearColor ATrueSkySequenceActor::GetMoonColor() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	FLinearColor col;
	if(!A)
		return col;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetTrueSkyPlugin()->GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		col.R=res.moonlight[0];
		col.G=res.moonlight[1];
		col.B=res.moonlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		col/=m;
	}
	return col;
}

float ATrueSkySequenceActor::GetSunIntensity() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	float i=1.0f;
	if(!A)
		return i;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetTrueSkyPlugin()->GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		FLinearColor col;
		col.R=res.sunlight[0];
		col.G=res.sunlight[1];
		col.B=res.sunlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		i=m;
	}
	return i;
}

float ATrueSkySequenceActor::GetMoonIntensity() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	float i=1.0f;
	if(!A)
		return i;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetTrueSkyPlugin()->GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		FLinearColor col;
		col.R=res.moonlight[0];
		col.G=res.moonlight[1];
		col.B=res.moonlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		i=m;
	}
	return i;
}

bool ATrueSkySequenceActor::UpdateLight(AActor *DirectionalLight,float multiplier,bool include_shadow,bool apply_rotation)
{
	return UpdateLight2(DirectionalLight, multiplier, multiplier, include_shadow,apply_rotation);
}

bool ATrueSkySequenceActor::UpdateLight2(AActor *DirectionalLight,float sun_multiplier,float moon_multiplier,bool include_shadow,bool apply_rotation)
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	if(!A)
		return false;
	if(!DirectionalLight)
		return false;
	TArray<UActorComponent*> comps=DirectionalLight->GetComponentsByClass(UDirectionalLightComponent::StaticClass());
	if(comps.Num()<=0)
		return false;
	UDirectionalLightComponent* DirectionalLightComponent = CastChecked<UDirectionalLightComponent>(comps[0]);
	if(!DirectionalLightComponent)
		return false;
	return( UpdateLightComponent2( DirectionalLightComponent, sun_multiplier,moon_multiplier, include_shadow, apply_rotation ) );
}

bool ATrueSkySequenceActor::UpdateLightComponent(ULightComponent *DirectionalLightComponent,float multiplier,bool include_shadow,bool apply_rotation)
{
	return UpdateLightComponent2(DirectionalLightComponent,multiplier,multiplier,include_shadow,apply_rotation);
}

bool ATrueSkySequenceActor::UpdateLightComponent2(ULightComponent *DirectionalLightComponent,float sun_multiplier,float moon_multiplier,bool include_shadow,bool apply_rotation)
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	if( !A || !IsValid( DirectionalLightComponent ) )
		return false;
	FVector pos(0,0,0);
	pos=DirectionalLightComponent->GetComponentTransform().GetTranslation();
	unsigned uid=(unsigned)((int64)DirectionalLightComponent);
	ITrueSkyPlugin::LightingQueryResult	res=GetTrueSkyPlugin()->GetLightingAtPosition(uid, pos);
	if(res.valid)
	{
		FLinearColor linearColour;
		linearColour.R=res.sunlight[0];
		linearColour.G=res.sunlight[1];
		linearColour.B=res.sunlight[2];
		linearColour*=sun_multiplier;
		if(include_shadow)
			linearColour*=res.sunlight[3];
		float m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
		float l=std::max(m,1.0f);
		float azimuth	=0.0f;	// Azimuth in compass degrees from north.
		float elevation	=0.0f;
		if(m>0.0f)
		{
			linearColour/=l;
			DirectionalLightComponent->SetCastShadows(true);
			DirectionalLightComponent->SetIntensity(l);
			FVector SunDirection=GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_SunDirection);
			azimuth				=180.0f/3.1415926536f*atan2(SunDirection.X,SunDirection.Y);
			elevation			=180.0f/3.1415926536f*asin(SunDirection.Z);
		}
		else
		{
			linearColour.R=res.moonlight[0];
			linearColour.G=res.moonlight[1];
			linearColour.B=res.moonlight[2];
			linearColour*=moon_multiplier;
			if(include_shadow)
				linearColour*=res.moonlight[3];
			m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
			l=std::max(m,1.0f);
			if(m>0.0f)
			{
				linearColour/=l;
				DirectionalLightComponent->SetCastShadows(true);
				DirectionalLightComponent->SetIntensity(l);
				FVector MoonDirection=GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_MoonDirection);
				azimuth				=180.0f/3.1415926536f*atan2(MoonDirection.X,MoonDirection.Y);
				elevation			=180.0f/3.1415926536f*asin(MoonDirection.Z);
			}
			else
			{
				DirectionalLightComponent->SetCastShadows(false);
				DirectionalLightComponent->SetIntensity(0.0f);
			}
		}
		DirectionalLightComponent->SetLightColor(linearColour,false);
		if(apply_rotation)
		{
			FRotator lightRotation(c1+m1*elevation,c2+m2*azimuth,c3+m3*elevation+m4*azimuth);
			tsa_dir			= lightRotation.Vector();
			FVector d_u		=TrueSkyToUEDirection(A->Transform,tsa_dir);
			float p		=asin(tsa_c*d_u.Z)*180.f/PI_F;
			float y		=atan2(tsa_a*d_u.Y,tsa_b*d_u.X)*180.f/PI_F;
			DirectionalLightComponent->bAbsoluteRotation = true;
			DirectionalLightComponent->SetWorldRotation(FRotator(p,y,0));
		}
		return true;
	}
	return false;
}
	
bool ATrueSkySequenceActor::UpdateSunlight(ALight *DirectionalLight,float multiplier,bool include_shadow,bool apply_rotation)
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	if(!A)
		return false;
	if(!DirectionalLight)
		return false;
	UDirectionalLightComponent* DirectionalLightComponent = CastChecked<UDirectionalLightComponent>(DirectionalLight->GetLightComponent());
	if(!DirectionalLightComponent)
		return false;
	FVector pos=A->Transform.GetTranslation();
	unsigned uid=(unsigned)((int64)DirectionalLight);
	ITrueSkyPlugin::LightingQueryResult	res=GetTrueSkyPlugin()->GetLightingAtPosition(uid, pos);
	if(res.valid)
	{
		FLinearColor linearColour;
		linearColour.R=res.sunlight[0];
		linearColour.G=res.sunlight[1];
		linearColour.B=res.sunlight[2];
		linearColour*=multiplier;
		if(include_shadow)
			linearColour*=res.sunlight[3];
		float m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
		float l=std::max(m,1.0f);
		if(m>0.0f)
		{
			linearColour/=l;
			DirectionalLightComponent->SetCastShadows(true);
			DirectionalLightComponent->SetIntensity(l);
		}
		else
		{
			DirectionalLightComponent->SetCastShadows(false);
			DirectionalLightComponent->SetIntensity(0.0f);
		}
		DirectionalLightComponent->SetLightColor(linearColour,false);
		if(apply_rotation)
		{
			float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunAzimuthDegrees");		// Azimuth in compass degrees from north.
			float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunElevationDegrees");
			FRotator sunRotation(c1+m1*elevation,c2+m2*azimuth,c3+m3*elevation+m4*azimuth);
	
			tsa_dir= sunRotation.Vector();
			FVector ts_u=TrueSkyToUEDirection(A->Transform,tsa_dir);
			float p	=asin(tsa_c*ts_u.Z)*180.f/PI_F;
			float y	=atan2(tsa_a*ts_u.Y,tsa_b*ts_u.X)*180.f/PI_F;
			DirectionalLight->SetActorRotation(FRotator(p,y,0));
		}
		return true;
	}
	return false;
}
	
bool ATrueSkySequenceActor::UpdateMoonlight(ALight *DirectionalLight,float multiplier,bool include_shadow,bool apply_rotation)
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	if(!A)
		return false;
	if(!DirectionalLight)
		return false;
	UDirectionalLightComponent* DirectionalLightComponent = CastChecked<UDirectionalLightComponent>(DirectionalLight->GetLightComponent());
	if(!DirectionalLightComponent)
		return false;
	FVector pos=A->Transform.GetTranslation();
	unsigned uid=(unsigned)((int64)DirectionalLight);
	ITrueSkyPlugin::LightingQueryResult	res=GetTrueSkyPlugin()->GetLightingAtPosition(uid, pos);
	if(res.valid)
	{
		FLinearColor linearColour;
		linearColour.R=res.moonlight[0];
		linearColour.G=res.moonlight[1];
		linearColour.B=res.moonlight[2];
		linearColour*=multiplier;
		if(include_shadow)
			linearColour*=res.moonlight[3];
		float m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
		float l=std::max(m,1.0f);
		if(m>0.0f)
		{
			linearColour/=l;
			DirectionalLightComponent->SetCastShadows(true);
			DirectionalLightComponent->SetIntensity(l);
		}
		else
		{
			DirectionalLightComponent->SetCastShadows(false);
			DirectionalLightComponent->SetIntensity(0.0f);
		}
		DirectionalLightComponent->SetLightColor(linearColour,false);
		if(apply_rotation)
		{
			float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonAzimuthDegrees");		// Azimuth in compass degrees from north.
			float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonElevationDegrees");
			FRotator sunRotation(c1+m1*elevation,c2+m2*azimuth,c3+m3*elevation+m4*azimuth);
	
			tsa_dir= sunRotation.Vector();
			tsa_u=TrueSkyToUEDirection(A->Transform,tsa_dir);
			float p	=asin(tsa_c*tsa_u.Z)*180.f/PI_F;
			float y	=atan2(tsa_a*tsa_u.Y,tsa_b*tsa_u.X)*180.f/PI_F;
			DirectionalLight->SetActorRotation(FRotator(p,y,0));
		}
		return true;
	}
	return false;
}

void ATrueSkySequenceActor::SetPointLightSource(int id,FLinearColor lc,float Intensity,FVector pos,float min_radius,float max_radius) 
{
	GetTrueSkyPlugin()->SetPointLight(id,lc*Intensity,pos, min_radius, max_radius);
}

void ATrueSkySequenceActor::SetPointLight(APointLight *source) 
{
	int id=(int)(int64_t)(source);
	FLinearColor lc=source->PointLightComponent->LightColor;
	lc*=source->PointLightComponent->Intensity;
	FVector pos=source->GetActorLocation();
	float min_radius=source->PointLightComponent->SourceRadius;
	float max_radius=source->PointLightComponent->AttenuationRadius;
	GetTrueSkyPlugin()->SetPointLight(id,lc,pos, min_radius, max_radius);
}

// Adapted from the function in SceneCaptureRendering.cpp.
void BuildProjectionMatrix(float InOrthoWidth,float FarPlane,FMatrix &ProjectionMatrix)
{
	const float OrthoSize = InOrthoWidth / 2.0f;

	const float NearPlane = 0;

	const float ZScale = 1.0f / (FarPlane - NearPlane);
	const float ZOffset = -NearPlane;

	ProjectionMatrix = FReversedZOrthoMatrix(
		OrthoSize,
		OrthoSize,
		ZScale,
		ZOffset
	);
}

static void AdaptOrthoProjectionMatrix(FMatrix &projMatrix, float metresPerUnit)
{
	projMatrix.M[0][0]	/= metresPerUnit;
	projMatrix.M[1][1]	/= metresPerUnit;
	projMatrix.M[2][2]	*= -1.0f/metresPerUnit;
	projMatrix.M[3][2]	= (1.0-projMatrix.M[3][2])/metresPerUnit;
}


void CaptureComponentToViewMatrix(FMatrix &viewMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix)
{
	FMatrix u=worldToSkyMatrix*viewMatrix;
	u.M[3][0]	*= metresPerUnit;
	u.M[3][1]	*= metresPerUnit;
	u.M[3][2]	*= metresPerUnit;
	// A camera in UE has X forward, Y right and Z up. This is no good for a view matrix, which needs to have Z forward
	// or backwards.
	static float U=0.f,V=90.f,W=0.f;	// pitch, yaw, roll.
	FRotator rot(U,V,W);
	FMatrix v;
	
	FRotationMatrix RotMatrix(rot);
	FMatrix r	=RotMatrix.GetTransposed();
	v			=r.operator*(u);
	
	{
		v.M[0][0]*=-1.f;
		v.M[0][1]*=-1.f;
		v.M[0][3]*=-1.f;
	}
	{
		v.M[1][2]*=-1.f;
		v.M[2][2]*=-1.f;
		v.M[3][2]*=-1.f;
	}
	viewMatrix.M[0][2]=v.M[0][0];
	viewMatrix.M[1][2]=v.M[1][0];
	viewMatrix.M[2][2]=v.M[2][0];
	viewMatrix.M[3][2]=v.M[3][0];

	viewMatrix.M[0][0]=v.M[0][1];
	viewMatrix.M[1][0]=v.M[1][1];
	viewMatrix.M[2][0]=v.M[2][1];
	viewMatrix.M[3][0]=v.M[3][1];

	viewMatrix.M[0][1]=v.M[0][2];
	viewMatrix.M[1][1]=v.M[1][2];
	viewMatrix.M[2][1]=v.M[2][2];
	viewMatrix.M[3][1]=v.M[3][2];

	viewMatrix.M[0][3]=v.M[0][3];
	viewMatrix.M[1][3]=v.M[1][3];
	viewMatrix.M[2][3]=v.M[2][3];
	viewMatrix.M[3][3]=v.M[3][3];
}

extern void AdaptViewMatrix(FMatrix &viewMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix);

void ATrueSkySequenceActor::TransferProperties()
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(!A)
		return;
	A->RainDepthRT					=nullptr;
	if (!IsActiveActor())
	{
		if(!IsAnyActorActive())
		{
			A->Playing=false;
			A->Visible					=false;
			A->activeSequence			=nullptr;
		}
		return;
	}
	SimulVersion simulVersion		=GetTrueSkyPlugin()->GetVersion();
	bSimulVersion4_1				=(simulVersion>SIMUL_4_0);
	A->InterpolationMode			=(uint8_t)InterpolationMode;
	A->InterpolationTimeSeconds		=InterpolationTimeSeconds;
	A->InterpolationTimeHours		=InterpolationTimeHours;
	A->RainNearThreshold			=RainNearThreshold;
	A->RenderWater					=RenderWater;
	A->EnableBoundlessOcean			=EnableBoundlessOcean;
	A->OceanBeaufortScale			=OceanBeaufortScale;
	A->OceanWindDirection			=OceanWindDirection;
	A->OceanWindDependancy			=OceanWindDependency;
	A->OceanScattering				=OceanScattering;
	A->OceanAbsorption				=OceanAbsorption;
	A->OceanOffset					=-(GetActorLocation());
	A->AdvancedWaterOptions			=AdvancedWaterOptions;
	A->OceanWindSpeed				=OceanWindSpeed;
	A->OceanWaveAmplitude			=OceanWaveAmplitude;
	A->OceanChoppyScale				=OceanChoppyScale;
	A->OceanFoamHeight				=OceanFoamHeight;
	A->OceanFoamChurn				=OceanFoamChurn;
	A->ShareBuffersForVR			=ShareBuffersForVR;
	A->Destroyed					=false;
	A->Visible						=Visible;
	A->MetresPerUnit				=MetresPerUnit;
	A->Brightness					=Brightness;
	A->SimpleCloudShadowing			=SimpleCloudShadowing;
	UWorld *world					=GetWorld();
	A->GridRendering				=(IntegrationScheme==EIntegrationScheme::Grid);
	if(A->activeSequence!=ActiveSequence)
	{
		A->activeSequence				=ActiveSequence;
		if (!world|| world->WorldType == EWorldType::Editor)
			A->Reset=true;
	}
	if (!world|| world->WorldType == EWorldType::Editor)
	{
		if(A->activeSequence==ActiveSequence)
			A->Playing=false;
	}
	A->Playing				=bPlaying;
	A->PrecipitationOptions			=(VelocityStreaks?(uint8_t)PrecipitationOptionsEnum::VelocityStreaks:0)
		|(SimulationTime?(uint8_t)PrecipitationOptionsEnum::SimulationTime:0)
		|(!bPlaying?(uint8_t)PrecipitationOptionsEnum::Paused:0);
	A->SimpleCloudShadowSharpness	=SimpleCloudShadowSharpness;
	A->CloudThresholdDistanceKm		=CloudThresholdDistanceKm;
	A->DownscaleFactor				=DownscaleFactor;
	A->MaximumResolution			=MaximumResolution;
	A->DepthSamplingPixelRange		=DepthSamplingPixelRange;
	A->Amortization					=Amortization;
	A->AtmosphericsAmortization		=AtmosphericsAmortization;
	A->MoonTexture					=GetTextureRHIRef(MoonTexture);
	A->CosmicBackgroundTexture		=GetTextureRHIRef(CosmicBackgroundTexture);
	A->LossRT						=GetTextureRHIRef(LossRT);
	A->CloudVisibilityRT			=GetTextureRHIRef(CloudVisibilityRT);
	A->CloudShadowRT				=GetTextureRHIRef(CloudShadowRT);
	A->InscatterRT					=GetTextureRHIRef(InscatterRT);
	A->RainCubemap					=GetTextureRHIRef(RainCubemap);
	A->Time							=Time;
	A->SetTime						=bSetTime;
	A->MaxSunRadiance				=MaxSunRadiance;
	A->AdjustSunRadius				=AdjustSunRadius;
	// Note: This seems to be necessary, it's not clear why:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	A->Transform					=GetTransform();
	A->DepthBlending				=DepthBlending;
	A->MinimumStarPixelSize			=MinimumStarPixelSize;
	A->GodraysGrid[0]				=GodraysGrid[0];
	A->GodraysGrid[1]				=GodraysGrid[1];
	A->GodraysGrid[2]				=GodraysGrid[2];
	for(int i=0;i<4;i++)
	{
		simul::LightningBolt *L=&(A->lightningBolts[i]);
		if(L&&L->id!=0&&L->id!=latest_thunderbolt_id)
		{
			FVector pos(L->endpos[0],L->endpos[1],L->endpos[2]);
			float magnitude=L->brightness;
			FVector colour(L->colour[0],L->colour[1],L->colour[2]);
			latest_thunderbolt_id=L->id;
			if(ThunderSounds.Num())
			{
				for( FConstPlayerControllerIterator Iterator = world->GetPlayerControllerIterator(); Iterator; ++Iterator )
				{
					FVector listenPos,frontDir,rightDir;
					Iterator->Get()->GetAudioListenerPosition(listenPos,frontDir,rightDir);
					FVector offset		=listenPos-pos;
				
					FTimerHandle UnusedHandle;
					float dist			=offset.Size()*A->MetresPerUnit;
					static float vsound	=3400.29f;
					float delaySeconds	=dist/vsound;
					FTimerDelegate soundDelegate = FTimerDelegate::CreateUObject( this, &ATrueSkySequenceActor::PlayThunder, pos );
					GetWorldTimerManager().SetTimer(UnusedHandle,soundDelegate,delaySeconds,false);
				}
			}
		}
	}
}

void ATrueSkySequenceActor::PlayThunder(FVector pos)
{
	USoundBase *ThunderSound=ThunderSounds[FMath::Rand()%ThunderSounds.Num()];
	if(ThunderSound)
		UGameplayStatics::PlaySoundAtLocation(this,ThunderSound,pos,FMath::RandRange(0.5f,1.5f),FMath::RandRange(0.7f,1.3f),0.0f,ThunderAttenuation);
}


float ATrueSkySequenceActor::GetMetresPerUnit()
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	return A->MetresPerUnit;
}

bool ATrueSkySequenceActor::IsActiveActor()
{
	UWorld *world = GetWorld();
	if(HasAnyFlags(RF_Transient|RF_ClassDefaultObject|RF_ArchetypeObject))
		return false;
	// WorldType is often not yet initialized here, even though we're calling from PostLoad.
	if (!world||world->WorldType == EWorldType::Editor||world->WorldType==EWorldType::Inactive)
		return ActiveInEditor;
	return ( this == worldCurrentActiveSequenceActors[world] );
}

bool ATrueSkySequenceActor::IsAnyActorActive() 
{
	if(!GetOuter())
		return false;
	ULevel* Level=Cast<ULevel>(GetOuter());
	if(!Level)
		return false;
	if(Level->OwningWorld == nullptr)
		return false;
	UWorld *world = GetWorld();
	if(!HasAnyFlags(RF_Transient|RF_ClassDefaultObject|RF_ArchetypeObject))
		if (!world||world->WorldType == EWorldType::Editor||world->WorldType==EWorldType::Inactive)
			return true;
	if( worldCurrentActiveSequenceActors[world] != nullptr )
		return true;
	return false;
}


FBox ATrueSkySequenceActor::GetComponentsBoundingBox(bool bNonColliding) const
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
static FVector CloudShadowOriginKm(0,0,0);
static FVector CloudShadowScaleKm(0,0,0);

#include "AssetRegistryModule.h"

void ATrueSkySequenceActor::FillMaterialParameters()
{
	if (!ITrueSkyPlugin::IsAvailable()||ITrueSkyPlugin::Get().GetVersion()==ToSimulVersion(0,0,0))
		return;
	UWorld* World = GEngine->GetWorldFromContextObject(this,EGetWorldErrorMode::ReturnNull);
	if (World)
	{
		// Warning: don't delete the trueSkyMaterialParameterCollection in-game. But why would you ever do that?
		if(!TrueSkyMaterialParameterCollection)
		{
			if(TrueSkyPropertyFloatMaps.Num()==0)
			{
				InitializeTrueSkyPropertyMap( );
			}
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			TArray<FAssetData> AssetData;
			const UClass* C = UMaterialParameterCollection::StaticClass();
			AssetRegistryModule.Get().GetAssetsByClass(C->GetFName(), AssetData);
			for(auto &I:AssetData)
			{
				UMaterialParameterCollection * mpc= static_cast<UMaterialParameterCollection*>(I.GetAsset());
				const FString &str=(mpc->GetName());
				static FString tsmp(L"trueSkyMaterialParameters");
				if(mpc&&str==tsmp)
				{
					TrueSkyMaterialParameterCollection=mpc;
					break;
				}
			}
		}
	
		if(TrueSkyMaterialParameterCollection)
		{
			static float mix = 0.05;
			UMaterialParameterCollection * mpc= TrueSkyMaterialParameterCollection;
			if(mpc)
				{
					ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
					float MaxAtmosphericDistanceKm=GetSkyLayerFloat(ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_MaxDistanceKm);
					CloudShadowOriginKm=GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_CloudShadowOriginKm);
					FVector CloudShadowOrigin=1000.0f*TrueSkyToUEPosition(A->Transform,A->MetresPerUnit,CloudShadowOriginKm);
					CloudShadowScaleKm=GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_CloudShadowScaleKm);
					FVector CloudShadowScale=1000.0f/A->MetresPerUnit*TrueSkyToUEDirection(A->Transform,CloudShadowScaleKm);
					FVector pos=A->Transform.GetTranslation();
					wetness*=1.0-mix;
					wetness+=mix*GetFloatPropertyAtPosition(ETrueSkyPositionalPropertiesFloat::TSPROPFLOAT_Precipitation,pos,9183759);
					//ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
					UKismetMaterialLibrary::SetScalarParameterValue(this,mpc,L"MaxAtmosphericDistance",MaxAtmosphericDistanceKm*1000.0f/A->MetresPerUnit);
					UKismetMaterialLibrary::SetVectorParameterValue(this,mpc,L"CloudShadowOrigin",CloudShadowOrigin);
					UKismetMaterialLibrary::SetVectorParameterValue(this,mpc,L"CloudShadowScale",CloudShadowScale);
					UKismetMaterialLibrary::SetScalarParameterValue(this,mpc,L"LocalWetness",wetness);
			}
		}
	}
}

void ATrueSkySequenceActor::TickActor(float DeltaTime,enum ELevelTick TickType,FActorTickFunction& ThisTickFunction)
{
	if(initializeDefaults)
	{
		SetDefaultTextures();
		initializeDefaults=false;
	}
	CleanQueries();
	TArray<UDirectionalLightComponent*> lights;
	GetComponents<UDirectionalLightComponent>(lights);
	for(auto light:lights)
	{
		//if(light->IsActive())
		{
			UpdateLightComponent2(light,SunMultiplier,MoonMultiplier,false,DriveLightDirection);
		}
	}
	if( IsValid( Light ) )
	{
		if(!DriveLightDirection)
			SetSunRotation(Light->GetTransform().Rotator());
		TArray<UDirectionalLightComponent*> lights2;
		Light->GetComponents<UDirectionalLightComponent>(lights2);
		for(auto light:lights2)
		{
			UpdateLightComponent2(light,SunMultiplier,MoonMultiplier,false,DriveLightDirection);
		}
	}
	UWorld *world = GetWorld();
	FillMaterialParameters();
	if(RainMaskSceneCapture2D)
	{
		ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
		static bool cc=true;
		USceneCaptureComponent2D *sc=(USceneCaptureComponent2D*)RainMaskSceneCapture2D->GetCaptureComponent2D();
		if(sc->IsActive())
		{
			sc->bUseCustomProjectionMatrix=cc;
			float ZHeight = RainDepthExtent;
			sc->OrthoWidth=RainMaskWidth;
			FMatrix proj;
			BuildProjectionMatrix( sc->OrthoWidth, ZHeight,proj);
			FMatrix ViewMat=sc->GetComponentToWorld().ToInverseMatrixWithScale();
			CaptureComponentToViewMatrix(ViewMat,MetresPerUnit,A->Transform.ToMatrixWithScale());
			sc->CustomProjectionMatrix=proj;
			AdaptOrthoProjectionMatrix(proj,MetresPerUnit);
			// Problem, UE doesn't fill in depth according to the projection matrix, it does it in cm! Or Unreal Units.
			A->RainDepthTextureScale=1.0f/ZHeight;
			A->RainDepthMatrix	=ViewMat*proj;
			A->RainProjMatrix	=proj;
			A->RainDepthRT			=GetTextureRHIRef(sc->TextureTarget);
		}
	}
	// We DO NOT accept ticks from actors in Editor worlds.
	// For some reason, actors start ticking when you Play in Editor, the ones in the Editor and well as the ones in the PIE world.
	// This results in duplicate actors competing.
	if (!world || world->WorldType == EWorldType::Editor)
	{
		ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
		// But we do transfer the properties.
		if(A&&A->Playing)
		{
			A->Playing=bPlaying=false;
			TransferProperties();
		}
		return;
	}
	// Find out which trueSKY actor should be active. We should only do this once per frame, so this is inefficient at present.
	ATrueSkySequenceActor *CurrentActiveActor = NULL;
	ATrueSkySequenceActor* GlobalActor = NULL;
	APawn* playerPawn = UGameplayStatics::GetPlayerPawn(world, 0);
	FVector pos(0, 0, 0);
	if (playerPawn)
		pos = playerPawn->GetActorLocation();
	else if (GEngine)
	{
		APlayerController * pc = GEngine->GetFirstLocalPlayerController(world);
		if (pc)
		{
			if (pc->PlayerCameraManager)
			{
				pos = pc->PlayerCameraManager->GetCameraLocation();
			}
		}
	}
	bool any_visible=false;
	for (TActorIterator<ATrueSkySequenceActor> Itr(world); Itr; ++Itr)
	{
		ATrueSkySequenceActor* SequenceActor = *Itr;
		if (SequenceActor->Visible)
		{
			any_visible=true;
			break;
		}
	}
	for (TActorIterator<ATrueSkySequenceActor> Itr(world); Itr; ++Itr)
	{
		ATrueSkySequenceActor* SequenceActor = *Itr;
		FBox box = SequenceActor->GetComponentsBoundingBox(true);
		if (box.GetSize().IsZero())
		{
			GlobalActor = SequenceActor;
			continue;
		}
		if (any_visible&&!SequenceActor->Visible)
			continue;
		if (box.IsInside(pos))
			CurrentActiveActor = SequenceActor;
	}
	if (CurrentActiveActor == nullptr)
		CurrentActiveActor = GlobalActor;
	
	worldCurrentActiveSequenceActors[world] = CurrentActiveActor;
	if (!IsActiveActor())
	{
		if(!IsAnyActorActive())
		{
			ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			if(A)
			{
				A->Playing					=false;
				A->Visible					=false;
				A->activeSequence			=nullptr;
			}
		}
	}
	bPlaying=!world->IsPaused();
	TransferProperties();
	UpdateVolumes();
}

#if WITH_EDITOR

bool ATrueSkySequenceActor::CanEditChange(const UProperty* InProperty)const
{
	if (InProperty)
	{
		FString PropertyName = InProperty->GetName();

		// Disable InterpolationTimeSeconds if interpolation is FixedNumber or GameTime
		if ((InterpolationMode == EInterpolationMode::FixedNumber || InterpolationMode == EInterpolationMode::GameTime)&&
			PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, InterpolationTimeSeconds))
		{
			return false;
		}
		// Disable InterpolationTimeHours if interpolation is FixedTime or RealTime
		if ((InterpolationMode == EInterpolationMode::FixedNumber || InterpolationMode == EInterpolationMode::RealTime) &&
			PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, InterpolationTimeHours))
		{
			return false;
		}

		if (!ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
		{
			if (PropertyName == "RenderWater"
				|| PropertyName == "EnableBoundlessOcean"
				|| PropertyName == "OceanWindSpeed"
				|| PropertyName == "OceanWindDirection"
				|| PropertyName == "OceanWindDependancy"
				|| PropertyName == "OceanScattering"
				|| PropertyName == "OceanAbsorption"
				|| PropertyName == "WaveAmplitude"
				|| PropertyName == "ChoppyScale"
				|| PropertyName == "FoamHeight"
				|| PropertyName == "FoamChurn")
				return false;
		}

		if (!AdvancedWaterOptions)
		{
			if (PropertyName == "OceanWindSpeed"
				|| PropertyName == "OceanWaveAmplitude"
				|| PropertyName == "OceanChoppyScale"
				|| PropertyName == "OceanFoamHeight"
				|| PropertyName == "OceanFoamChurn")
				return false;
		}
	}
	return true;
}

void ATrueSkySequenceActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	TransferProperties();
	UpdateVolumes();
	// If this is the active Actor, deactivate the others:
	if (ActiveInEditor)
	{
		UWorld *world = GetWorld();
		if (!world||world->WorldType!= EWorldType::Editor)
		{
			return;
		}
		for (TActorIterator<ATrueSkySequenceActor> Itr(world); Itr; ++Itr)
		{
			ATrueSkySequenceActor* SequenceActor = *Itr;
			if (SequenceActor == this)
				continue;
			SequenceActor->ActiveInEditor = false;
		}
		worldCurrentActiveSequenceActors[world] = this;
		// Force instant, not gradual change, if the change was from editing:
		GetTrueSkyPlugin()->TriggerAction("Reset");
	}
}

void ATrueSkySequenceActor::EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	Super::EditorApplyTranslation(DeltaTranslation, bAltDown, bShiftDown, bCtrlDown);
	TransferProperties();
}
#endif

// Cached Modules
ITrueSkyPlugin *ATrueSkySequenceActor::CachedTrueSkyPlugin = nullptr;

ITrueSkyPlugin *ATrueSkySequenceActor::GetTrueSkyPlugin()
{
	if (!ATrueSkySequenceActor::CachedTrueSkyPlugin)
	{
		ATrueSkySequenceActor::CachedTrueSkyPlugin = &ITrueSkyPlugin::Get();
	}
	
	return ATrueSkySequenceActor::CachedTrueSkyPlugin;
}

void ATrueSkySequenceActor::PopulatePropertyFloatMap( UEnum* EnumerationType, TrueSkyPropertyFloatMap& EnumerationMap,FString prefix )
{
	// Enumerate through all of the values.
	for( int i = 0; i < EnumerationType->GetMaxEnumValue( ); ++i )
	{
		FString keyName = EnumerationType->GetNameByIndex( i ).ToString( );

		TArray< FString > parsedKeyName;
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check( ParsedElements > 0 );
		FString nameString=prefix+parsedKeyName[ParsedElements-1];

		int64 enum_int64=GetTrueSkyPlugin()->GetEnum(nameString);
#ifdef UE_LOG_SIMUL_INTERNAL
		if( !enum_int64 )
			UE_LOG_SIMUL_INTERNAL( TrueSky, Warning, TEXT( "No enum for Float: %s"), *nameString );
#endif	// UE_LOG_SIMUL_INTERNAL
		
		// The property name is the final key.
		EnumerationMap.Add( EnumerationType->GetValueByIndex( i ), TrueSkyPropertyFloatData( FName( *nameString ), 0.0f, 1.0f, enum_int64 ) );
	}
}

void ATrueSkySequenceActor::PopulatePropertyIntMap( UEnum* EnumerationType, TrueSkyPropertyIntMap& EnumerationMap ,FString prefix)
{
	// Enumerate through all of the values.
	for( int i = 0; i < EnumerationType->GetMaxEnumValue( ); ++i )
	{
		FString keyName = EnumerationType->GetNameByIndex( i ).ToString( );

		TArray< FString > parsedKeyName;
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check( ParsedElements > 0 );
		FString nameString=prefix+parsedKeyName[ParsedElements-1];
		// UL End

		int64 enum_int64=GetTrueSkyPlugin()->GetEnum(nameString);
#ifdef UE_LOG_SIMUL_INTERNAL
		if( !enum_int64 )
			UE_LOG_SIMUL_INTERNAL( TrueSky, Warning, TEXT( "No enum for Int: %s" ), *nameString );
#endif	// UE_LOG_SIMUL_INTERNAL

//		UE_LOG( TrueSky, Warning, TEXT( "Registering: %s"), *parsedKeyName[2] );

		// The property name is the final key.
		EnumerationMap.Add( EnumerationType->GetValueByIndex( i ), TrueSkyPropertyIntData( FName( *nameString), 0, 1, enum_int64 ) );
	}
}

void ATrueSkySequenceActor::PopulatePropertyVectorMap( UEnum* EnumerationType, TrueSkyPropertyVectorMap& EnumerationMap ,FString prefix)
{
	// Enumerate through all of the values.
	for( int i = 0; i < EnumerationType->GetMaxEnumValue( ); ++i )
	{
		FString keyName = EnumerationType->GetNameByIndex( i ).ToString( );

		TArray< FString > parsedKeyName;
		// UL Begin - @jeff.sult: fix crash in shipping build
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check( ParsedElements > 0 );
		FString nameString=prefix+parsedKeyName[ParsedElements-1];
		// UL End

		int64 enum_int64 = ITrueSkyPlugin::Get( ).GetEnum( nameString );
#ifdef UE_LOG_SIMUL_INTERNAL
		if( !enum_int64 )
			UE_LOG_SIMUL_INTERNAL( TrueSky, Warning, TEXT( "No enum for Vector: %s" ), *nameString );
#endif	// UE_LOG_SIMUL_INTERNAL

		//		UE_LOG( TrueSky, Warning, TEXT( "Registering: %s"), *parsedKeyName[2] );

		// The property name is the final key.
		if(enum_int64!=0)
			EnumerationMap.Add( EnumerationType->GetValueByIndex( i ), TrueSkyPropertyVectorData( FName( *nameString), enum_int64 ) );
	}
}

void ATrueSkySequenceActor::InitializeTrueSkyPropertyMap( )
{
	TrueSkyPropertyFloatMaps.Empty( );
	TrueSkyPropertyIntMaps.Empty( );
	TrueSkyPropertyVectorMaps.Empty( );


	// Setup the enumerations.
	TArray< FName > enumerationNames = {
		TEXT( "ETrueSkySkySequenceLayerPropertiesFloat" ),
		TEXT( "ETrueSkySkySequenceKeyframePropertiesFloat" ),
		TEXT( "ETrueSkyCloudSequenceLayerPropertiesFloat" ),
		TEXT( "ETrueSkyCloudSequenceKeyframePropertiesFloat" ),
		TEXT( "ETrueSkySkySequenceLayerPropertiesInt" ),
		TEXT( "ETrueSkySkySequenceKeyframePropertiesInt" ),
		TEXT( "ETrueSkyCloudSequenceLayerPropertiesInt" ),
		TEXT( "ETrueSkyCloudSequenceKeyframePropertiesInt" ),
		TEXT( "ETrueSkyPropertiesFloat" ),
		TEXT( "ETrueSkyPropertiesInt" ),
		TEXT( "ETrueSkyPositionalPropertiesFloat" ),
		TEXT( "ETrueSkyPropertiesVector" )
	};

	TArray< int32* > enumerationIndices = {
		&TSSkyLayerPropertiesFloatIdx,
		&TSSkyKeyframePropertiesFloatIdx,
		&TSCloudLayerPropertiesFloatIdx,
		&TSCloudKeyframePropertiesFloatIdx,
		&TSSkyLayerPropertiesIntIdx,
		&TSSkyKeyframePropertiesIntIdx,
		&TSCloudLayerPropertiesIntIdx,
		&TSCloudKeyframePropertiesIntIdx,
		&TSPropertiesFloatIdx,
		&TSPropertiesIntIdx,
		&TSPositionalPropertiesFloatIdx,
		&TSPropertiesVectorIdx,
	};

	// Populate the full enumeration property map.
	for( int i = 0; i < enumerationNames.Num( ); ++i )
	{
		FName enumerator = enumerationNames[i];
		FString prefix="";
		UEnum* pEnumeration = FindObject< UEnum >( ANY_PACKAGE, *enumerator.ToString( ), true );
		if(enumerator==TEXT( "ETrueSkySkySequenceLayerPropertiesFloat" )
			||enumerator==TEXT( "ETrueSkySkySequenceLayerPropertiesInt"))
			prefix="sky:";
		else if(enumerator==TEXT( "ETrueSkyCloudSequenceLayerPropertiesFloat" )
			||enumerator==TEXT( "ETrueSkyCloudSequenceLayerPropertiesInt"))
			prefix="clouds:";
		if( enumerator.ToString( ).Contains( TEXT( "Float" ) ) )
		{
			// Float-based map population.
			int32 idx = TrueSkyPropertyFloatMaps.Add( TrueSkyPropertyFloatMap( ) );
			*enumerationIndices[i] = idx;

			PopulatePropertyFloatMap( pEnumeration, TrueSkyPropertyFloatMaps[idx],prefix);
		}
		else if( enumerator.ToString( ).Contains( TEXT( "Vector" ) ) )
		{
			// Float-based map population.
			int32 idx = TrueSkyPropertyVectorMaps.Add( TrueSkyPropertyVectorMap( ) );
			*enumerationIndices[i] = idx;

			PopulatePropertyVectorMap( pEnumeration, TrueSkyPropertyVectorMaps[idx],prefix);
		}
		else
		{
			// Int-based map population.
			int32 idx = TrueSkyPropertyIntMaps.Add( TrueSkyPropertyIntMap( ) );
			*enumerationIndices[i] = idx;

			PopulatePropertyIntMap( pEnumeration, TrueSkyPropertyIntMaps[idx],prefix);
		}
	}
	auto p=ATrueSkySequenceActor::TrueSkyPropertyFloatMaps[ATrueSkySequenceActor::TSPropertiesFloatIdx].Find( ( int64 )ETrueSkyPropertiesFloat::TSPROPFLOAT_DepthSamplingPixelRange );

	{
		// Set the float property min/max.
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_BrightnessPower, 0.01f, 1.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_MaxAltitudeKm, 2.0f, 100.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_MaxDistanceKm, 5.0f, 2000.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_OvercastEffectStrength, 0.0f, 1.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_OzoneStrength, 0.0f, 0.2f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_Emissivity, 0.0f, 1.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_SunRadiusArcMinutes, 0.0f, 3600.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_SunBrightnessLimit, 1.0f, 10000000.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_LatitudeRadians, -1.57f, 1.57f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_LongitudeRadians, -3.14159f, 3.14159f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_TimezoneHours, -12.0f, 12.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_MoonAlbedo, 0.0f, 1.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_MoonRadiusArcMinutes, 0.0f, 3600.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_MaxStarMagnitude, 0.0f, 9.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_StarBrightness, 1.0f, 10000.0f );
		SetFloatPropertyData( TSSkyLayerPropertiesFloatIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLT_SkyLayer_BackgroundBrightness, 0.000001f, 0.001f );

		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_Daytime, 0.0f, std::numeric_limits< float >::max( ) );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_SeaLevelTemperatureK, -273.0f, 1000.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_Haze, 0.00001f, 1000.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_HazeBaseKm, -2.0f, 20.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_HazeScaleKm, 0.1f, 10.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_HazeEccentricity, 0.0f, 1.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_MieRed, 0.0f, 1.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_MieGreen, 0.0f, 1.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_MieBlue, 0.0f, 1.0f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_SunElevation, -1.57f, 1.57f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_SunAzimuth, -3.14159f, 3.14159f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_MoonElevation, -1.57f, 1.57f );
		SetFloatPropertyData( TSSkyKeyframePropertiesFloatIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesFloat::TSPROPFLT_SkyKeyframe_MoonAzimuth, -3.14159f, 3.14159f );

		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_PrecipitationThresholdKm, 0.0f, 10.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_PrecipitationRadiusMetres, 0.0f, 100.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_RainFallSpeedMS, 0.0f, 120.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_RainDropSizeMm, 0.001f, 100.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_CloudShadowStrength, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_CloudShadowRange, 0.5f, 200.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_MaxCloudDistanceKm, 100.0f, 500.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_NoisePeriod, 0.001f, 1000.0f );
		SetFloatPropertyData( TSCloudLayerPropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesFloat::TSPROPFLT_CloudLayer_EdgeNoisePersistence, 0.0f, 1.0f );

		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Cloudiness, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_CloudBase, -1.0f, 20.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_CloudHeight, 0.1f, 25.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_CloudWidthKm, 20.0f, 200.0f );
		//		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_CloudWidthMetres, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_WindSpeed, 0.0f, 1000.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_WindHeadingDegrees, 0.0f, 360.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_DistributionBaseLayer, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_DistributionTransition, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_UpperDensity, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Persistence, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_WorleyNoise, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_WorleyScale, 0.0f, 12.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Diffusivity, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_DirectLight, 0.0f, 4.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_LightAsymmetry, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_IndirectLight, 0.0f, 4.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_AmbientLight, 0.0f, 4.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Extinction, 0.0001f, 10.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_GodrayStrength, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_FractalWavelength, 1.0f, 1000.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_FractalAmplitude, 0.01f, 100.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_EdgeWorleyScale, 0.1f, 10000.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_EdgeWorleyNoise, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_EdgeSharpness, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_BaseNoiseFactor, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Churn, 0.001f, 1000.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_RainRadiusKm, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Precipitation, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_RainEffectStrength, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_RainToSnow, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_PrecipitationWindEffect, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_PrecipitationWaver, 0.0f, 1.0f );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Offsetx, std::numeric_limits< float >::min( ), std::numeric_limits< float >::max( ) );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_Offsety, std::numeric_limits< float >::min( ), std::numeric_limits< float >::max( ) );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_RainCentreXKm, std::numeric_limits< float >::min( ), std::numeric_limits< float >::max( ) );
		SetFloatPropertyData( TSCloudKeyframePropertiesFloatIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesFloat::TSPROPFLT_CloudKeyframe_RainCentreYKm, std::numeric_limits< float >::min( ), std::numeric_limits< float >::max( ) );
	}

	{
		// Set int property data.
		SetIntPropertyData( TSSkyLayerPropertiesIntIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesInt::TSPROPINT_SkyLayer_NumAltitudes, 1, 8 );
		SetIntPropertyData( TSSkyLayerPropertiesIntIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesInt::TSPROPINT_SkyLayer_NumElevations, 4, 256 );
		SetIntPropertyData( TSSkyLayerPropertiesIntIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesInt::TSPROPINT_SkyLayer_NumDistances, 4, 128 );
		SetIntPropertyData( TSSkyLayerPropertiesIntIdx, ( int64 )ETrueSkySkySequenceLayerPropertiesInt::TSPROPINT_SkyLayer_StartDayNumber, 0, std::numeric_limits< int32 >::max( ) );

		SetIntPropertyData( TSSkyKeyframePropertiesIntIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesInt::TSPROPINT_SkyKeyframe_StoreAsColours, 0, 1 );
		SetIntPropertyData( TSSkyKeyframePropertiesIntIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesInt::TSPROPINT_SkyKeyframe_NumColourAltitudes, 1, 8 );
		SetIntPropertyData( TSSkyKeyframePropertiesIntIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesInt::TSPROPINT_SkyKeyframe_NumColourElevations, 1, 15 );
		SetIntPropertyData( TSSkyKeyframePropertiesIntIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesInt::TSPROPINT_SkyKeyframe_NumColourDistances, 1, 15 );
		SetIntPropertyData( TSSkyKeyframePropertiesIntIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesInt::TSPROPINT_SkyKeyframe_AutoMie, 0, 1 );
		SetIntPropertyData( TSSkyKeyframePropertiesIntIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesInt::TSPROPINT_SkyKeyframe_AutomaticSunPosition, 0, 1 );
		SetIntPropertyData( TSSkyKeyframePropertiesIntIdx, ( int64 )ETrueSkySkySequenceKeyframePropertiesInt::TSPROPINT_SkyKeyframe_AutomaticMoonPosition, 0, 1 );

		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_Visible, 0, 1 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_Wrap, 0, 1 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_OverrideWind, 0, 1 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_RenderMode, 0, 1 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_MaxPrecipitationParticles, 0, 1000000 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_DefaultNumSlices, 80, 255 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_ShadowTextureSize, 16, 256 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_GridWidth, 16, 512 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_GridHeight, 1, 64 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_NoiseResolution, 4, 64 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_EdgeNoiseFrequency, 1, 16 );
		SetIntPropertyData( TSCloudLayerPropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceLayerPropertiesInt::TSPROPINT_CloudLayer_EdgeNoiseResolution, 4, 64 );

		SetIntPropertyData( TSCloudKeyframePropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesInt::TSPROPINT_CloudKeyframe_Octaves, 1, 5 );
		SetIntPropertyData( TSCloudKeyframePropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesInt::TSPROPINT_CloudKeyframe_RegionalPrecipitation, 0, 1 );
		SetIntPropertyData( TSCloudKeyframePropertiesIntIdx, ( int64 )ETrueSkyCloudSequenceKeyframePropertiesInt::TSPROPINT_CloudKeyframe_LockRainToClouds, 0, 1 );
	}
}

void ATrueSkySequenceActor::RenderToCubemap(UTrueSkyLightComponent *TrueSkyLightComponent,UTextureRenderTargetCube *RenderTargetCube)
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(A)
	{
		A->CaptureCubemapRT=GetTextureRHIRef(RenderTargetCube);
		A->CaptureCubemapTrueSkyLightComponent=TrueSkyLightComponent;
		TrueSkyLightComponent->ResetTrueSkyLight();
	}
}

static void ShowTrueSkyMemory(const TArray<FString>& Args, UWorld* World)
{
	FString str = ATrueSkySequenceActor::GetText("memory");
	GEngine->AddOnScreenDebugMessage(1, 5.f, FColor(255, 255, 0, 255), str);
	UE_LOG(TrueSky, Display, TEXT("%s"), *str);
}

static FAutoConsoleCommandWithWorldAndArgs GTrueSkyMemory(
	TEXT("r.TrueSky.Memory"),
	TEXT("Show how much GPU memory trueSKY is using."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowTrueSkyMemory)
);

static void ShowCloudCrossSections(const TArray<FString>& Args, UWorld* World)
{
	bool value = false;
	if(Args.Num()>0)
		value=(FCString::Atoi(*Args[0]))!=0;
	ATrueSkySequenceActor::SetBool("ShowCloudCrossSections",value);
}

static FAutoConsoleCommandWithWorldAndArgs GTrueSkyCrossSections(
	TEXT("r.TrueSky.CloudCrossSections"),
	TEXT("Show the cross-sections of the cloud volumes."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowCloudCrossSections)
);


static void ShowCompositing(const TArray<FString>& Args, UWorld* World)
{
	bool value = false;
	if (Args.Num()>0)
		value = (FCString::Atoi(*Args[0])) != 0;
	ATrueSkySequenceActor::SetBool("ShowCompositing", value);
}

static FAutoConsoleCommandWithWorldAndArgs GTrueSkyCompositing(
	TEXT("r.TrueSky.Compositing"),
	TEXT("Show the compositing overlay for trueSKY."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowCompositing)
);
