// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyPluginPrivatePCH.h"
#include "TrueSkySequenceAsset.h"
#include "TrueSkySequenceActor.h"
#include "TrueSkyWaterActor.h"
#define INCLUDE_UE_EDITOR_FEATURES 0
#undef IsMaximized
#undef IsMinimized
#include "GenericWindow.h"

#include <string>
#include <vector>

#include "RendererInterface.h"
#include "TrueSkyLightComponent.h"
#include "DynamicRHI.h"
#include "RHIResources.h"
#include "PlatformFilemanager.h"
#include "FileManager.h"
#include "UnrealClient.h"
#include <map>
#include <algorithm>
#include <wchar.h>
#include "ModifyDefinitions.h"
#include "TrueSkySettings.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#include "Classes/Engine/Canvas.h"
#include "Classes/Engine/TextureRenderTarget2D.h"

#include "TrueSkySequencePropertiesFloat.h"
#include "TrueSkySequencePropertiesInt.h"
bool simul::internal_logs=false;
// Until Epic updates FPostOpaqueRenderDelegate, define FRenderDelegateParameters to make equivalent, if not using the Simul branch of UE.
typedef FPostOpaqueRenderParameters FRenderDelegateParameters;
typedef FPostOpaqueRenderDelegate FRenderDelegate;

#if PLATFORM_SWITCH
extern int errno;
#endif
#ifndef PLATFORM_UWP
#define PLATFORM_UWP 0
#endif
#ifndef USE_FAST_SEMANTICS_RENDER_CONTEXTS
#define USE_FAST_SEMANTICS_RENDER_CONTEXTS 0
#endif

#if PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_PS4 || PLATFORM_UWP
#define TRUESKY_PLATFORM_SUPPORTED 1
#else
#define TRUESKY_PLATFORM_SUPPORTED 0
#endif

#if PLATFORM_WINDOWS || PLATFORM_XBOXONE 
#define BREAK_IF_DEBUGGING if ( IsDebuggerPresent()) DebugBreak();
#elif PLATFORM_UWP 
#define BREAK_IF_DEBUGGING if ( IsDebuggerPresent() ) __debugbreak( );
#else
#if PLATFORM_PS4
#define BREAK_IF_DEBUGGING SCE_BREAK();
#else
#define BREAK_IF_DEBUGGING
#endif
#endif
#define ERRNO_CHECK \
if (errno != 0)\
		{\
		static bool checked=false;\
		if(!checked)\
		{\
			BREAK_IF_DEBUGGING\
			checked=true;\
		}\
		errno = 0; \
		}

#define ERRNO_CLEAR \
if (errno != 0)\
		{\
			errno = 0;\
		}
	
#if PLATFORM_PS4
#include <kernel.h>
#include <gnm.h>
#include <gnm/common.h>
#include <gnm/error.h>
#include <gnm/shader.h>
#include "GnmRHI.h"
#include "GnmRHIPrivate.h"
#include "GnmMemory.h"
#include "GnmContext.h"

#elif PLATFORM_SWITCH
#define USE_FAST_SEMANTICS_RENDER_CONTEXTS 0
#include <errno.h>
#define strcpy_s(a, b, c) strcpy(a, c)
#define wcscpy_s(a, b, c) wcscpy(a, c)
#define wcscat_s(a, b, c) wcscat(a, c)
#endif

// Dependencies.
#include "RHI.h"
#include "GPUProfiler.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "SceneUtils.h"	// For SCOPED_DRAW_EVENT
#include "StaticArray.h"
#include "ActorCrossThreadProperties.h"
#include "WaterActorCrossThreadProperties.h"
#include "Allocator.h"
#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

#define LOCTEXT_NAMESPACE "TrueSkyPlugin"
#define LLOCTEXT_NAMESPACE L"TrueSkyPlugin"
/** This is a macro that casts a dynamically bound RHI reference to the appropriate D3D type. */
/** This is a macro that casts a dynamically bound RHI reference to the appropriate D3D type. */
#define DYNAMIC_CAST_D3D11RESOURCE(Type,Name) \
	FD3D11##Type* Name = (FD3D11##Type*)Name##RHI;

DEFINE_LOG_CATEGORY(TrueSky);

#if INCLUDE_UE_EDITOR_FEATURES
DECLARE_LOG_CATEGORY_EXTERN(LogD3D11RHI, Log, All);
#endif
#if PLATFORM_LINUX || PLATFORM_MAC
#include <wchar.h>
#endif
#if PLATFORM_MAC
#endif


#if PLATFORM_WINDOWS || PLATFORM_UWP
#include "../Private/Windows/D3D11RHIBasePrivate.h"
#include "D3D11Util.h"
#include "D3D11RHIPrivate.h"
#include "D3D11State.h"
#include "D3D11Resources.h"
typedef unsigned __int64 uint64_t;
typedef FD3D11DynamicRHI FD3D11CommandListContext;
#if !SIMUL_BINARY_PLUGIN
#include "D3D12/TrueSkyPluginD3D12.h"
#endif
#endif

#if PLATFORM_XBOXONE
// No more D3D11 for Xbox One.
#if !SIMUL_BINARY_PLUGIN
#include "D3D12/TrueSkyPluginD3D12.h"
#endif
#endif

#include "Tickable.h"
#include "EngineModule.h"

using namespace simul;
using namespace unreal;
namespace simul
{
	namespace unreal
	{
		ActorCrossThreadProperties *actorCrossThreadProperties=nullptr;
		ActorCrossThreadProperties *GetActorCrossThreadProperties()
		{
			return actorCrossThreadProperties;
		}
	}
}
TMap<int, WaterActorCrossThreadProperties*> waterActorsCrossThreadProperties;
WaterActorCrossThreadProperties* GetWaterActorCrossThreadProperties(int ID)
{
	return( ( waterActorsCrossThreadProperties.Contains( ID ) ) ? waterActorsCrossThreadProperties[ID] : nullptr );
}
void AddWaterActorCrossThreadProperties(WaterActorCrossThreadProperties* newProperties)
{	
	waterActorsCrossThreadProperties.Add( newProperties->ID, newProperties );
}
void RemoveWaterActorCrossThreadProperties(int ID)
{
	waterActorsCrossThreadProperties.Remove( ID );
}

void *GetPlatformDevice()
{
#if PLATFORM_PS4
	// PS4 has no concept of separate devices. For "device" we will specify the immediate context's BaseGfxContext.
	FRHICommandListImmediate& CommandList = FRHICommandListExecutor::GetImmediateCommandList();
	FGnmCommandListContext *ct=(FGnmCommandListContext*)(&CommandList.GetContext());
	sce::Gnmx::LightweightGfxContext *bs=&(ct->GetContext());
	//bs->validate();
	void * device =(void*)bs;
#else
	void * device =GDynamicRHI->RHIGetNativeDevice();
#endif
	return device;
}

void* GetPlatformContext(FRenderDelegateParameters& RenderParameters, GraphicsDeviceType api)
{
	void *pContext=nullptr;
#if PLATFORM_PS4
	FGnmCommandListContext *ctx=(FGnmCommandListContext*)(&RenderParameters.RHICmdList->GetContext());
	sce::Gnmx::LightweightGfxContext *lwgfxc=&(ctx->GetContext());
	pContext=lwgfxc;

#elif ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (api == GraphicsDeviceD3D12)
	{
		return GetContextD3D12(RenderParameters.RHICmdList->GetDefaultContext());
	}
#endif
	return pContext;
}

namespace simul
{
	namespace base
	{
		// Note: the following definition MUST exactly mirror the one in Simul/Base/FileLoader.h, but we wish to avoid includes from external projects,
		//  so it is reproduced here.
		class FileLoader
		{
		public:
			//! Returns a pointer to the current file handler.
			static FileLoader *GetFileLoader();
			//! Returns true if and only if the named file exists. If it has a relative path, it is relative to the current directory.
			virtual bool FileExists(const char *filename_utf8) const = 0;
			//! Set the file handling object: call this before any file operations, if at all.
			static void SetFileLoader(FileLoader *f);
			//! Put the file's entire contents into memory, by allocating sufficiently many bytes, and setting the pointer.
			//! The memory should later be freed by a call to \ref ReleaseFileContents.
			//! The filename should be unicode UTF8-encoded.
			virtual void AcquireFileContents(void*& pointer, unsigned int& bytes, const char* filename_utf8, bool open_as_text) = 0;
			//! Get the file date as a julian day number. Return zero if the file doesn't exist.
			virtual double GetFileDate(const char* filename_utf8) = 0;
			//! Free the memory allocated by AcquireFileContents.		
			virtual void ReleaseFileContents(void* pointer) = 0;
			//! Save the chunk of memory to storage.
			virtual bool Save(void* pointer, unsigned int bytes, const char* filename_utf8, bool save_as_text) = 0;
		};
	}
	namespace unreal
	{
		class UE4FileLoader:public base::FileLoader
		{
			static FString ProcessFilename(const char *filename_utf8)
			{
				FString Filename(UTF8_TO_TCHAR(filename_utf8));
#if PLATFORM_PS4
				Filename = Filename.ToLower();
#endif
				Filename = Filename.Replace(UTF8_TO_TCHAR("\\"), UTF8_TO_TCHAR("/"));
				Filename = Filename.Replace(UTF8_TO_TCHAR("//"), UTF8_TO_TCHAR("/"));
				return Filename;
			}
		public:
			UE4FileLoader()
			{}
			virtual ~UE4FileLoader()
			{
			}
			virtual bool FileExists(const char *filename_utf8) const override
			{
				FString Filename = ProcessFilename(filename_utf8);
				//UE_LOG(TrueSky,Display,TEXT("Checking FileExists %s"),*Filename);
				bool result=false;
				if(!Filename.IsEmpty())
				{
					result = FPlatformFileManager::Get().GetPlatformFile().FileExists(*Filename);
					// Now errno may be nonzero, which is unwanted.
					errno = 0;
				}
				//UE_LOG(TrueSky,Display,TEXT("Checking FileExists %s"),*Filename);
				return result;
			}
			virtual void AcquireFileContents(void*& pointer, unsigned int& bytes, const char* filename_utf8, bool open_as_text) override
			{
				pointer = nullptr;
				bytes = 0;
				if (!FileExists(filename_utf8))
				{
					return;
				}
				FString Filename = ProcessFilename(filename_utf8);
				ERRNO_CLEAR
				IFileHandle *fh = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*Filename);
				if (!fh)
					return;
				ERRNO_CLEAR
				bytes = fh->Size();
				// We append a zero, in case it's text. UE4 seems to not distinguish reading binaries or strings.
				pointer = new uint8[bytes+1];
				fh->Read((uint8*)pointer, bytes);
				((uint8*)pointer)[bytes] = 0;
				delete fh;
				ERRNO_CLEAR
			}
			virtual double GetFileDate(const char* filename_utf8) override
			{
				FString Filename	=ProcessFilename(filename_utf8);
				FDateTime dt		=FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*Filename);
				int64 uts			=dt.ToUnixTimestamp();
				double time_s		=( double(uts)/ 86400.0 );
				// Don't use FDateTime's GetModifiedJulianDay, it's only accurate to the nearest day!!
				return time_s;
			}
			virtual void ReleaseFileContents(void* pointer) override
			{
				delete [] ((uint8*)pointer);
			}
			virtual bool Save(void* pointer, unsigned int bytes, const char* filename_utf8, bool save_as_text) override
			{
				FString Filename = ProcessFilename(filename_utf8);
				IFileHandle *fh = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Filename);
				if (!fh)
				{
					errno=0;
					return false;
				}
				fh->Write((const uint8*)pointer, bytes);
				delete fh;
				errno=0;
				return true;
			}
		};
	}
}
using namespace simul;
using namespace unreal;

static void SetMultiResConstants(TArray<FVector2D> &MultiResConstants,FSceneView *View)
{
#ifdef NV_MULTIRES
	FMultiRes::RemapCBData RemapCBData;
	FMultiRes::CalculateRemapCBData(&View->MultiResConf, &View->MultiResViewports, &RemapCBData);
	
	 MultiResConstants.Add(RemapCBData.MultiResToLinearSplitsX);
	 MultiResConstants.Add(RemapCBData.MultiResToLinearSplitsY);
	
	for (int32 i = 0; i < 3; ++i)
	{
		MultiResConstants.Add(RemapCBData.MultiResToLinearX[i]);
    }
    for (int32 i = 0; i < 3; ++i)
    {
		MultiResConstants.Add(RemapCBData.MultiResToLinearY[i]);
    }
	MultiResConstants.Add(RemapCBData.LinearToMultiResSplitsX);
	MultiResConstants.Add(RemapCBData.LinearToMultiResSplitsY);
    for (int32 i = 0; i < 3; ++i)
    {
		MultiResConstants.Add(RemapCBData.LinearToMultiResX[i]);
    }
    for (int32 i = 0; i < 3; ++i)
    {
            MultiResConstants.Add(RemapCBData.LinearToMultiResY[i]);
    }
    check(FMultiRes::Viewports::Count == 9);
    for (int32 i = 0; i < FMultiRes::Viewports::Count; ++i)
    {
		const FViewportBounds& Viewport = View->MultiResViewportArray[i];
		MultiResConstants.Add(FVector2D(Viewport.TopLeftX, Viewport.TopLeftY));
		MultiResConstants.Add(FVector2D(Viewport.Width, Viewport.Height));
    }
    for (int32 i = 0; i < FMultiRes::Viewports::Count; ++i)
    {
		const FIntRect& ScissorRect = View->MultiResScissorArray[i];
		MultiResConstants.Add(FVector2D(ScissorRect.Min));
		MultiResConstants.Add(FVector2D(ScissorRect.Max));
    }
#endif
}

static simul::unreal::UE4FileLoader ue4SimulFileLoader;

#if PLATFORM_PS4
typedef int moduleHandle;
#else
typedef void* moduleHandle;
#endif

#if !TRUESKY_PLATFORM_SUPPORTED
template<typename T> void *GetPlatformTexturePtr(T *t2, GraphicsDeviceType api)
{
	return nullptr;
}
#elif PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_UWP
	typedef void* PlatformRenderTarget;
	typedef void* PlatformDepthStencilTarget;
	typedef void* PlatformTexture;

PlatformTexture GetPlatformTexturePtr(FTextureRHIRef t, GraphicsDeviceType api)
{
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (api == GraphicsDeviceD3D12)
	{
		return t.GetReference()?t.GetReference()->GetNativeResource():nullptr;
		//return (void*)GetPlatformTexturePtrD3D12(t2d);
	}
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP  ) 
	ID3D11Texture2D *T = nullptr;
	FD3D11TextureBase *m = static_cast<FD3D11Texture2D*>(t.GetReference());
	if (m)
	{
		T = (ID3D11Texture2D*)(m->GetResource());
	}
	return T;
#endif
	return nullptr;
}

void* GetPlatformTexturePtr(FRHITexture2D *t2, GraphicsDeviceType api)
{
	if(!t2)
		return nullptr;
	return t2->GetNativeResource();
}

void* GetPlatformTexturePtr(FRHITexture3D *t2, GraphicsDeviceType api)
{
	if(!t2)
		return nullptr;
	return t2->GetNativeResource();
}

void* GetPlatformRenderTarget(FRHITexture2D* t2d, GraphicsDeviceType api)
{
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (api == GraphicsDeviceD3D12)
	{
		return (void*)GetRenderTargetD3D12(t2d);
	}
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP ) && !SIMUL_BINARY_PLUGIN
	ID3D11RenderTargetView *T=nullptr;
	FD3D11TextureBase *m = static_cast<FD3D11Texture2D*>(t2d);
	if(m)
	{
		T = (ID3D11RenderTargetView*)(m->GetRenderTargetView(0,0));
			return (void*)T;
	}
#endif
		return nullptr;
}

void* GetPlatformDepthStencilTarget(FRHITexture2D* t2d, GraphicsDeviceType api)
{
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (api == GraphicsDeviceD3D12)
	{
		return (void*)GetRenderTargetD3D12(t2d);
	}
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP ) && !SIMUL_BINARY_PLUGIN
	ID3D11DepthStencilView *T = nullptr;
	FD3D11TextureBase *m = static_cast<FD3D11Texture2D*>(t2d);
	if (m)
	{
		T = (ID3D11DepthStencilView*)m->GetDepthStencilView(FExclusiveDepthStencil(FExclusiveDepthStencil::Type::DepthWrite_StencilWrite));
			return (void*)T;
	}
#endif
	return nullptr;
}

void* GetPlatformTexturePtr(UTexture *t, GraphicsDeviceType api)
{
	if (t&&t->Resource)
	{
		FRHITexture2D *t2 = static_cast<FRHITexture2D*>(t->Resource->TextureRHI.GetReference());
		return GetPlatformTexturePtr(t2,api);
	}
	return nullptr;
}
	
void* GetPlatformTexturePtr(FTexture *t, GraphicsDeviceType api)
{
	if (t)
	{
		FRHITexture2D *t2 = static_cast<FRHITexture2D*>(t->TextureRHI.GetReference());
		return GetPlatformTexturePtr(t2,api);
	}
	return nullptr;
}
#endif

#if PLATFORM_PS4
typedef sce::Gnm::Texture PlatformTexture;
typedef sce::Gnm::RenderTarget PlatformRenderTarget;
typedef sce::Gnm::DepthRenderTarget PlatformDepthStencilTarget;

sce::Gnm::Texture *GetPlatformTexturePtr(FTextureRHIRef t, GraphicsDeviceType api)
{
	sce::Gnm::Texture *T=nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t.GetReference());
	if (m)
	{
		T = (sce::Gnm::Texture*)(m->Surface.Texture);
	}
	return T;
}

sce::Gnm::Texture *GetPlatformTexturePtr(FRHITexture2D *t2, GraphicsDeviceType api)
{
	sce::Gnm::Texture *T=nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t2);
	if(m)
	{
		T = (sce::Gnm::Texture*)(m->Surface.Texture);
	}
	return T;
}

PlatformTexture *GetPlatformTexturePtr(FTexture *t, GraphicsDeviceType api)
{
	if (t)
	{
		FRHITexture2D *t2 = static_cast<FRHITexture2D*>(t->TextureRHI.GetReference());
		return GetPlatformTexturePtr(t2,api);
	}
	return nullptr;
}

	PlatformRenderTarget *GetPlatformRenderTarget(FRHITexture2D *t2d)
{
	sce::Gnm::RenderTarget *T=nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t2d);
	if(m)
	{
		T = (sce::Gnm::RenderTarget*)(m->Surface.ColorBuffer);
	}
	return T;
}

PlatformDepthStencilTarget *GetPlatformDepthStencilTarget(FRHITexture2D *t2d)
{
	sce::Gnm::DepthRenderTarget *T = nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t2d);
	if (m)
	{
		T = (sce::Gnm::DepthRenderTarget*)(m->Surface.DepthBuffer);
	}
	return T;
}
#endif

//! Returns a view to the RenderTarget in RenderParameters.
void* GetColourTarget(FRenderDelegateParameters& RenderParameters, GraphicsDeviceType CurrentRHIType)
{
	void* target = nullptr;
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (CurrentRHIType == GraphicsDeviceD3D11 || CurrentRHIType==GraphicsDeviceD3D11_FastSemantics)
	{
		target = (void*)GetPlatformRenderTarget(RenderParameters.RenderTargetTexture, CurrentRHIType);
		// RenderParameters.RenderTargetTexture->AddRef();
	}
	else if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		target = (void*)GetRenderTargetD3D12(RenderParameters.RenderTargetTexture);
		// RenderParameters.RenderTargetTexture->AddRef();
	}
#elif PLATFORM_PS4
	target = (void*)GetPlatformRenderTarget(RenderParameters.RenderTargetTexture);
#endif
	return target;
}

#include "RHIUtilities.h"




void StoreUEState(FRenderDelegateParameters& RenderParameters)
{
	if(RenderParameters.DepthTexture)
		RenderParameters.DepthTexture->AddRef();
#if !SIMUL_BINARY_PLUGIN
		RenderParameters.RenderTargetTexture->AddRef();
#endif
}

#if PLATFORM_PS4
void RestoreClipState(FGnmCommandListContext *GnmCommandListContext)
{
	GnmContextType *GnmContext=&GnmCommandListContext->GetContext();
	sce::Gnm::ClipControl Clip;
	Clip.init();
	Clip.setClipSpace( Gnm::kClipControlClipSpaceDX );

	// ISR multi-view writes to S_VIEWPORT_INDEX in vertex shaders
	static const auto CVarInstancedStereo = IConsoleManager::Get().FindTConsoleVariableDataInt( TEXT( "vr.InstancedStereo" ) );
	static const auto CVarMultiView = IConsoleManager::Get().FindTConsoleVariableDataInt( TEXT( "vr.MultiView" ) );

	if ( CVarInstancedStereo &&
		CVarInstancedStereo->GetValueOnAnyThread() &&
		CVarMultiView &&
		CVarMultiView->GetValueOnAnyThread() )
	{
		Clip.setForceViewportIndexFromVsEnable( true );
	}

	GnmContext->setClipControl( Clip );

	Gnm::DbRenderControl DBRenderControl;
	DBRenderControl.init();
	GnmContext->setDbRenderControl( DBRenderControl );

	// Can't access these values, let's hope no-one changed them.
	GnmContext->setDepthBoundsRange( 0.0f,1.0f);//CachedDepthBoundsMin, CachedDepthBoundsMax );
}
#endif

//! Restores the rendering state 
void RestoreUEState(FRenderDelegateParameters& RenderParameters, GraphicsDeviceType CurrentRHIType)
{
	// Release the textures
	RenderParameters.DepthTexture->Release();
#if !SIMUL_BINARY_PLUGIN
	RenderParameters.RenderTargetTexture->Release();
#endif
#if PLATFORM_PS4
	if ( CurrentRHIType == GraphicsDevicePS4)
	{
		FGnmCommandListContext* CommandListContext = (FGnmCommandListContext*)(&RenderParameters.RHICmdList->GetContext());
		RestoreClipState(CommandListContext);
		CommandListContext->RestoreCachedDCBState();
	}
#endif

#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		RestoreHeapsD3D12(&RenderParameters.RHICmdList->GetContext());
	}
#endif
}

#ifndef UE_LOG_ONCE
#define UE_LOG_ONCE(a,b,c) {static bool done=false; if(!done) {UE_LOG(a,b,c, TEXT(""));done=true;}}
#endif

static void FlipFlapMatrix(FMatrix &v,bool flipx,bool flipy,bool flipz,bool flapx,bool flapy,bool flapz)
{
	if(flipx)
	{
		v.M[0][0]*=-1.f;
		v.M[0][1]*=-1.f;
		v.M[0][2]*=-1.f;
		v.M[0][3]*=-1.f;
	}
	if(flipy)
	{
		v.M[1][0]*=-1.f;
		v.M[1][1]*=-1.f;
		v.M[1][2]*=-1.f;
		v.M[1][3]*=-1.f;
	}
	if(flipz)
	{
		v.M[2][0]*=-1.f;
		v.M[2][1]*=-1.f;
		v.M[2][2]*=-1.f;
		v.M[2][3]*=-1.f;
	}
	if(flapx)
	{
		v.M[0][0]*=-1.f;
		v.M[1][0]*=-1.f;
		v.M[2][0]*=-1.f;
		v.M[3][0]*=-1.f;
	}
	if(flapy)
	{
		v.M[0][1]*=-1.f;
		v.M[1][1]*=-1.f;
		v.M[2][1]*=-1.f;
		v.M[3][1]*=-1.f;
	}
	if(flapz)
	{
		v.M[0][2]*=-1.f;
		v.M[1][2]*=-1.f;
		v.M[2][2]*=-1.f;
		v.M[3][2]*=-1.f;
	}
}

static void AdaptProjectionMatrix(FMatrix &projMatrix, float metresPerUnit)
{
	projMatrix.M[2][0]	*=-1.0f;
	projMatrix.M[2][1]	*=-1.0f;
	projMatrix.M[2][3]	*=-1.0f;
	projMatrix.M[3][2]	*= metresPerUnit;
}
// Bizarrely, Res = Mat1.operator*(Mat2) means Res = Mat2^T * Mat1, as
 //* opposed to Res = Mat1 * Mat2.
//  Equally strangely, the view matrix we get from rendering is NOT the same orientation as the one from the Editor Viewport class.
void AdaptScaledMatrix(FMatrix &sMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix)
{
	FMatrix u=worldToSkyMatrix*sMatrix;
	for(int i=0;i<4;i++)
	{
		u.M[i][0]	*= metresPerUnit;
		u.M[i][1]	*= metresPerUnit;
		u.M[i][2]	*= metresPerUnit;
	}
	// Swap y and x - no good reason why UE needs this, but it does.
	std::swap(u.M[3][0],u.M[3][1]);
	static float U=90.f,V=90.f,W=0.f;
	static float Ue=0.f,Ve=0.f,We=0.f;
	FRotator rot(U,V,W);
	FMatrix v;
	{
		FRotationMatrix RotMatrix(rot);
		FMatrix r	=RotMatrix.GetTransposed();
		v			=r.operator*(u);
		static bool x=true,y=false,z=false,X=false,Y=false,Z=false;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
	}
	sMatrix=v;
}

// NOTE: What Unreal calls "y", we call "x". This is because trueSKY uses the engineering standard of a right-handed coordinate system,
// Whereas UE uses the graphical left-handed coordinates.
//
void AdaptViewMatrix(FMatrix &viewMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix)
{
	FMatrix u=worldToSkyMatrix*viewMatrix;
	u.M[3][0]	*= metresPerUnit;
	u.M[3][1]	*= metresPerUnit;
	u.M[3][2]	*= metresPerUnit;
	static float U=0.f,V=90.f,W=0.f;
	FRotator rot(U,V,W);
	FMatrix v;
	{
		FRotationMatrix RotMatrix(rot);
		FMatrix r	=RotMatrix.GetTransposed();
		v			=r.operator*(u);
		static bool x=true,y=false,z=false,X=false,Y=false,Z=true;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
	}
	viewMatrix=v;
}

// Just an ordinary transformation matrix: we must convert it from UE's left-handed system to right-handed for trueSKY
static void RescaleMatrix(FMatrix &viewMatrix,float metresPerUnit)
{
	viewMatrix.M[3][0]	*= metresPerUnit;
	viewMatrix.M[3][1]	*= metresPerUnit;
	viewMatrix.M[3][2]	*= metresPerUnit;
	static bool fulladapt=true;
	if(fulladapt)
	{
		static float U=0.f,V=-90.f,W=0.f;
		FRotationMatrix RotMatrix(FRotator(U,V,W));
		FMatrix r=RotMatrix.GetTransposed();
		FMatrix v=viewMatrix.operator*(r);		// i.e. r * viewMatrix
		static bool postm=true;
		if(postm)
			v=RotMatrix*v;
		
	
		static bool x=true,y=false,z=false,X=true,Y=false,Z=false;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
		static bool inv=true;
		if(inv)
			v=v.Inverse();
		else
			v=v;
		viewMatrix=v;
		return;
	}
}
static void AdaptCubemapMatrix(FMatrix &viewMatrix)
{
	static float U=0.f,V=90.f,W=0.f;
	FRotationMatrix RotMatrix(FRotator(U,V,W));
	FMatrix r=RotMatrix.GetTransposed();
	FMatrix v=viewMatrix.operator*(r);
	static bool postm=true;
	if(postm)
		v=r.operator*(viewMatrix);
	{
		static bool x=true,y=false,z=false,X=false,Y=false,Z=true;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
	}
	{
		static bool x=true,y=true,z=false,X=false,Y=false,Z=false;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
	}
	viewMatrix=v;
}


#define ENABLE_AUTO_SAVING


static std::wstring Utf8ToWString(const char *src_utf8)
{
	int src_length=(int)strlen(src_utf8);
#ifdef _MSC_VER
	int length = MultiByteToWideChar(CP_UTF8, 0, src_utf8,src_length, 0, 0);
#else
	int length=src_length;
#endif
	wchar_t *output_buffer = new wchar_t [length+1];
#ifdef _MSC_VER
	MultiByteToWideChar(CP_UTF8, 0, src_utf8, src_length, output_buffer, length);
#else
	mbstowcs(output_buffer, src_utf8, (size_t)length );
#endif
	output_buffer[length]=0;
	std::wstring wstr=std::wstring(output_buffer);
	delete [] output_buffer;
	return wstr;
}
static std::string WStringToUtf8(const wchar_t *src_w)
{
	int src_length=(int)wcslen(src_w);
#ifdef _MSC_VER
	int size_needed = WideCharToMultiByte(CP_UTF8, 0,src_w, (int)src_length, nullptr, 0, nullptr, nullptr);
#else
	int size_needed=2*src_length;
#endif
	char *output_buffer = new char [size_needed+1];
#ifdef _MSC_VER
	WideCharToMultiByte (CP_UTF8,0,src_w,(int)src_length,output_buffer, size_needed, nullptr, nullptr);
#else
	wcstombs(output_buffer, src_w, (size_t)size_needed );
#endif
	output_buffer[size_needed]=0;
	std::string str_utf8=std::string(output_buffer);
	delete [] output_buffer;
	return str_utf8;
}
static std::string FStringToUtf8(const FString &Source)
{
	std::string str_utf8;
	const wchar_t *src_w=(const wchar_t*)(Source.GetCharArray().GetData());
	if(!src_w)
		return str_utf8;
	int src_length=(int)wcslen(src_w);
#ifdef _MSC_VER
	int size_needed = WideCharToMultiByte(CP_UTF8, 0,src_w, (int)src_length, nullptr, 0, nullptr, nullptr);
#else
	int size_needed=2*src_length;
#endif
	char *output_buffer = new char [size_needed+1];
#ifdef _MSC_VER
	WideCharToMultiByte (CP_UTF8,0,src_w,(int)src_length,output_buffer, size_needed, nullptr, nullptr);
#else
	wcstombs(output_buffer, src_w, (size_t)size_needed );
#endif
	output_buffer[size_needed]=0;
	str_utf8=std::string(output_buffer);
	delete [] output_buffer;
	return str_utf8;
}

static FString Utf8ToFString(const char *src_utf8)
{
	int src_length=(int)strlen(src_utf8);
#ifdef _MSC_VER
	int length = MultiByteToWideChar(CP_UTF8, 0, src_utf8,src_length, 0, 0);
#else
	int length=src_length;
#endif
	wchar_t *output_buffer = new wchar_t [length+1];
#ifdef _MSC_VER
	MultiByteToWideChar(CP_UTF8, 0, src_utf8, src_length, output_buffer, length);
#else
	mbstowcs(output_buffer, src_utf8, (size_t)length );
#endif
	output_buffer[length]=0;
	FString wstr=FString(output_buffer);
	delete [] output_buffer;
	return wstr;
}
#define DECLARE_TOGGLE(name)\
	void					OnToggle##name();\
	bool					IsToggled##name();

#define IMPLEMENT_TOGGLE(name)\
	void FTrueSkyPlugin::OnToggle##name()\
{\
	if(StaticGetRenderBool!=nullptr&&StaticSetRenderBool!=nullptr)\
	{\
		bool current=StaticGetRenderBool(#name);\
		StaticSetRenderBool(#name,!current);\
	}\
}\
\
bool FTrueSkyPlugin::IsToggled##name()\
{\
	if(StaticGetRenderBool!=nullptr)\
		if(StaticGetRenderBool)\
			return StaticGetRenderBool(#name);\
	return false;\
}

#define DECLARE_ACTION(name)\
	void					OnTrigger##name()\
	{\
		if(StaticTriggerAction!=nullptr)\
			StaticTriggerAction(#name);\
	}


class FTrueSkyTickable : public  FTickableGameObject
{
public:
	/** Tick interface */
	void					Tick( float DeltaTime );
	bool					IsTickable() const;
	TStatId					GetStatId() const;
};

struct Variant32
{
	union
	{
		float floatVal;
		int32 intVal;
	};
};

//! A simple variant for passing small data
struct Variant
{
	union
	{
		float Float;
		int32 Int;
		double Double;
		int64 Int64;
		vec3 Vec3;
	};
};

class FTrueSkyPlugin : public ITrueSkyPlugin
#ifdef SHARED_FROM_THIS
	, public TSharedFromThis<FTrueSkyPlugin,(ESPMode::Type)0>
#endif
{
	FCriticalSection criticalSection;
	bool GlobalOverlayFlag;
	bool bIsInstancedStereoEnabled;
	bool post_trans_registered;
	TMap<int32,FTextureRHIRef> skylightTargetTextures;
public:
	FTrueSkyPlugin();
	virtual ~FTrueSkyPlugin();

	static FTrueSkyPlugin*	Instance;
	void					OnDebugTrueSky(class UCanvas* Canvas, APlayerController*);

	/** IModuleInterface implementation */
	void					StartupModule() override;
	void					ShutdownModule() override;
	bool					SupportsDynamicReloading() override;

	/** Render delegates */
	void					DelegatedRenderFrame( FRenderDelegateParameters& RenderParameters );
	void					DelegatedRenderPostTranslucent( FRenderDelegateParameters& RenderParameters );
	void					DelegatedRenderOverlays(FRenderDelegateParameters& RenderParameters);
	void					RenderFrame(uint64_t uid,FRenderDelegateParameters& RenderParameters,EStereoscopicPass StereoPass,bool bMultiResEnabled,int FrameNumber);
	void					RenderPostTranslucent(uint64_t uid,FRenderDelegateParameters& RenderParameters,EStereoscopicPass StereoPass,bool bMultiResEnabled,int FrameNumber);
	void					RenderOverlays(uint64_t uid,FRenderDelegateParameters& RenderParameters,int FrameNumber);
	
	// This updates the TrueSkyLightComponents i.e. trueSky Skylights
	void					UpdateTrueSkyLights(FRenderDelegateParameters& RenderParameters,int FrameNumber);

	/** Init rendering */
	bool					InitRenderingInterface(  );

	/** Enable rendering */
	void					SetRenderingEnabled( bool Enabled );

	/** Enable water rendering*/
	void					SetWaterRenderingEnabled(bool Enabled);

	/** Check for sequence actor to disable access to water variables*/
	bool					GetWaterRenderingEnabled();

	/** If there is a TrueSkySequenceActor in the persistent level, this returns that actor's TrueSkySequenceAsset */
	UTrueSkySequenceAsset*	GetActiveSequence();
	void UpdateFromActor();
	
	void					*GetRenderEnvironment() override;
	bool					TriggerAction(const char *name);
	bool					TriggerAction(const FString &fname) override;
	static void				LogCallback(const char *txt);
	
	void					SetPointLight(int id,FLinearColor c,FVector pos,float min_radius,float max_radius) override;

	float					GetCloudinessAtPosition(int32 queryId,FVector pos) override;
	VolumeQueryResult		GetStateAtPosition(int32 queryId,FVector pos) override;
	LightingQueryResult		GetLightingAtPosition(int32 queryId,FVector pos) override;
	float					CloudLineTest(int32 queryId,FVector StartPos,FVector EndPos) override;

	void					SetRenderBool(const FString &fname, bool value) override;
	bool					GetRenderBool(const FString &fname) const override;

	void					SetRenderFloat(const FString &fname, float value) override;
	float					GetRenderFloat(const FString &fname) const override;
	float					GetRenderFloatAtPosition(const FString &fname,FVector pos) const override;
	float					GetFloatAtPosition(int64 Enum,FVector pos,int32 uid) const override;
	float					GetFloat(int64 Enum) const override;
	FVector					GetVector(int64 Enum) const override;
	void					SetVector(int64 Enum,FVector value) const override;
	
	void					SetRender(const FString &fname,const TArray<FVariant> &params) override;
	void					SetRenderInt(const FString& name, int value) override;
	int						GetRenderInt(const FString& name) const override;
	int						GetRenderInt(const FString& name,const TArray<FVariant> &params) const override;
	
	void					SetRenderString(const FString &fname, const FString & value) override;
	FString					GetRenderString(const FString &fname) const override;

	void					SetKeyframeFloat(unsigned uid,const FString &fname, float value) override;
	float					GetKeyframeFloat(unsigned uid,const FString &fname) const override;
	void					CreateBoundedWaterObject(unsigned int ID, float* dimension, float* location) override;
	void					RemoveBoundedWaterObject(unsigned int ID) override;
							   
	virtual void			SetWaterFloat(const FString &fname, unsigned int ID, float value) override;
	virtual void			SetWaterInt(const FString &fname, unsigned int ID, int value) override;
	virtual void			SetWaterBool(const FString &fname, unsigned int ID, bool value) override;
	virtual void			SetWaterVector(const FString &fname, int ID, FVector value) override;
	virtual float			GetWaterFloat(const FString &fname, unsigned int ID)  const override;
	virtual int				GetWaterInt(const FString &fname, unsigned int ID) const override;
	virtual bool			GetWaterBool(const FString &fname, unsigned int ID) const override;
	virtual float*			GetWaterVector(const FString &fname, unsigned int ID) const override;
							   
	void					SetKeyframeInt(unsigned uid,const FString& name, int value) override;
	int						GetKeyframeInt(unsigned uid,const FString& name) const override;
	
	void					SetCloudShadowRenderTarget(FRenderTarget *t);

	void SetInt( ETrueSkyPropertiesInt Property, int32 Value );
	void SetVector( ETrueSkyPropertiesVector Property, FVector Value );
	void SetFloat( ETrueSkyPropertiesFloat Property, float Value );

	FMatrix UEToTrueSkyMatrix(bool apply_scale=true) const;
	FMatrix TrueSkyToUEMatrix(bool apply_scale=true) const;
	virtual UTexture *GetAtmosphericsTexture()
	{
		return AtmosphericsTexture;
	}
	struct CloudVolume
	{
		FTransform transform;
		FVector extents;
	};
	virtual void ClearCloudVolumes()
	{
	criticalSection.Lock();
		cloudVolumes.clear();
	criticalSection.Unlock();
	}
	virtual void SetCloudVolume(int i,FTransform tr,FVector ext)
	{
	criticalSection.Lock();
		cloudVolumes[i].transform=tr;
		cloudVolumes[i].extents=ext;
	criticalSection.Unlock();
	}
	
	virtual int32 SpawnLightning(FVector startpos,FVector endpos,float magnitude,FVector colour) override
	{
		startpos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,startpos);
		endpos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,endpos);
		return StaticSpawnLightning(((const float*)(&startpos)),((const float*)(&endpos)) , magnitude,((const float*)(&colour)) );
	}
	virtual void RequestColourTable(unsigned uid,int x,int y,int z) override
	{
		if(colourTableRequests.find(uid)==colourTableRequests.end()||colourTableRequests[uid]==nullptr)
			colourTableRequests[uid]=new ColourTableRequest;

		ColourTableRequest *req=colourTableRequests[uid];
		
		req->uid=uid;
		req->x=x;
		req->y=y;
		req->z=z;
	}
	virtual const ColourTableRequest *GetColourTable(unsigned uid) override
	{
		auto it=colourTableRequests.find(uid);
		if(it==colourTableRequests.end())
			return nullptr;
		if(it->second->valid==false)
			return nullptr;
		return it->second;
	}
	virtual void ClearColourTableRequests() override
	{
		for(auto i:colourTableRequests)
		{
			delete [] i.second->data;
			delete i.second;
		}
		colourTableRequests.clear();
	}
	virtual SimulVersion GetVersion() const override
	{
		return simulVersion;
	}
	int64	GetEnum(const FString &name) const override;
protected:
	std::map<unsigned,ColourTableRequest*> colourTableRequests;
	std::map<int,CloudVolume> cloudVolumes;

	void					PlatformSetGraphicsDevice();
	bool					EnsureRendererIsInitialized();
	
	UTexture				*AtmosphericsTexture;
	FTexture3DRHIRef CloudTexture3D;	// for Xbox One ESRAM
	void					RenderCloudShadow();
	void					OnMainWindowClosed(const TSharedRef<SWindow>& Window);

	/** Called when Toggle rendering button is pressed */
	void					OnToggleRendering();

	void					OnUIChangedSequence();
	void					OnUIChangedTime(float t);

	void					ExportCloudLayer(const FString& filenameUtf8,int index);

	/** Called when the Actor is pointed to a different sequence.*/
	void					SequenceChanged();
	/** Returns true if Toggle rendering button should be enabled */
	bool					IsToggleRenderingEnabled();
	/** Returns true if Toggle rendering button should be checked */
	bool					IsToggleRenderingChecked();

	/** Called when the Toggle Fades Overlay button is pressed*/
	DECLARE_TOGGLE(ShowFades)
	DECLARE_TOGGLE(ShowCompositing)
	DECLARE_TOGGLE(ShowWaterTextures)
	DECLARE_TOGGLE(Show3DCloudTextures)
	DECLARE_TOGGLE(Show2DCloudTextures)

	void ShowDocumentation()
	{
	//	FString DocumentationUrl = FDocumentationLink::ToUrl(Link);
		FString DocumentationUrl="https://docs.simul.co/unrealengine";
		FPlatformProcess::LaunchURL(*DocumentationUrl, nullptr, nullptr);
	}
	
	/** Adds a TrueSkySequenceActor to the current scene */
	void					OnAddSequence();
	void					OnSequenceDestroyed();
	/** Returns true if user can add a sequence actor */
	bool					IsAddSequenceEnabled();

	/** Initializes all necessary paths */
	void					InitPaths();
	
	struct int4
	{
		int x,y,z,w;
	};
	struct Viewport
	{
		Viewport()
			: x(0)
			, y(0)
			, w(0)
			, h(0)
			, zmin(0.f)
			, zmax(1.f)
		{
		}
		int x,y,w,h;
		float zmin,zmax;
	};
	
	struct RenderFrameStruct //MUST be kept in sync with any plugin version that supports this UE version
	{
		void* device;
		void *pContext;
		int view_id;
		const float* viewMatrix4x4;
		const float* projMatrix4x4;
		void* depthTexture;
		void* colourTarget;
		void* colourTargetTexture;
		bool isColourTargetTexture;
		int4 depthViewport;
		Viewport *targetViewport;
		PluginStyle style;
		float exposure, gamma, framenumber;
		const float *nvMultiResConstants;
	};

	void SetEditorCallback(FTrueSkyEditorCallback c) override
	{
		TrueSkyEditorCallback=c;
	}
	void InitializeDefaults(ATrueSkySequenceActor *a) override
	{
		a->initializeDefaults=true;
	}

	void AdaptScaledMatrix(FMatrix &sMatrix);
	void AdaptViewMatrix(FMatrix &viewMatrix,bool editor_version=false);
	
	typedef int (*FStaticInitInterface)(  );
	typedef int (*FGetSimulVersion)(int *major,int *minor,int *build);
	typedef void(*FStaticSetMemoryInterface)(simul::base::MemoryInterface *);
	typedef int (*FStaticShutDownInterface)(  );
	typedef void (*FLogCallback)(const char *);
	typedef void (*FStaticSetDebugOutputCallback)(FLogCallback);
	typedef void (*FStaticSetGraphicsDevice)(void* device, GraphicsDeviceType deviceType,GraphicsEventType eventType);
	typedef void (*FStaticSetGraphicsDeviceAndContext)(void* device,void* context, GraphicsDeviceType deviceType, GraphicsEventType eventType);
	typedef int (*FStaticPushPath)(const char*,const char*);
	typedef void(*FStaticSetFileLoader)(simul::base::FileLoader *);
	typedef int (*FStaticGetOrAddView)( void *);
	typedef int (*FStaticGetLightningBolts)(LightningBolt *,int maxb);
	typedef int (*FStaticSpawnLightning)(const float startpos[3],const float endpos[3],float magnitude,const float colour[3]);
	typedef int (*FStaticRenderFrame)(void* device,void* deviceContext,int view_id
		,float* viewMatrix4x4
		,float* projMatrix4x4
		,void* depthTexture
		,void* depthResource
		,int4 depthViewport
		,const Viewport *v
		,PluginStyle s
		,float exposure
		,float gamma
		,int framenumber
		,const float *nvMultiResConstants);
	typedef int(*FStaticRenderFrame2)(RenderFrameStruct* frameStruct);
	typedef int (*FStaticCopySkylight)(void *pContext
												,int cube_id
												,float* shValues
												,int shOrder
												,void *targetTex
												,const float *engineToSimulMatrix4x4
												,int updateFrequency
												,float blend
												,float copy_exposure
												,float copy_gamma);
	typedef int (*FStaticCopySkylight2)(void *pContext
												,int cube_id
												,float* shValues
												,int shOrder
												,void *targetTex
												,const float *engineToSimulMatrix4x4
												,int updateFrequency
												,float blend
												,float copy_exposure
												,float copy_gamma
												,const float *ground_colour);
	typedef int (*FStaticProbeSkylight)(void *pContext
												,int cube_id
												,int mip_size
												,int face_index
												,int x
												,int y
												,int w
												,int h
												,float *targetValues);
	typedef int (*FStaticRenderOverlays)(void* device,void* deviceContext,void* depthTexture,
				 const float* viewMatrix4x4,const float* projMatrix4x4,int view_id, void *colourTarget, const Viewport *viewports);
	typedef int (*FStaticTick)( float deltaTime );
	typedef int (*FStaticOnDeviceChanged)( void * device );
	typedef void* (*FStaticGetEnvironment)();
	typedef int (*FStaticSetSequence)( std::string sequenceInputText );
	typedef class UE4PluginRenderInterface* (*FStaticGetRenderInterfaceInstance)();
	typedef void (*FStaticSetPointLight)(int id,const float pos[3],float radius,float maxradius,const float radiant_flux[3]);
	typedef void (*FStaticCloudPointQuery)(int id,const float *pos, VolumeQueryResult *res);
	typedef void (*FStaticLightingQuery)(int id,const float *pos, LightingQueryResult *res);
	typedef void (*FStaticCloudLineQuery)(int id,const float *startpos,const float *endpos, LineQueryResult *res);
	
	typedef void(*FStaticSetRenderTexture)(const char *name, void* texturePtr);
	typedef void(*FStaticSetMatrix4x4)(const char *name, const float []);
	
	typedef void (*FStaticSetRender)( const char *name,int num_params,const Variant32 *params);
	typedef void (*FStaticSetRenderBool)( const char *name,bool value );
	typedef bool (*FStaticGetRenderBool)( const char *name );
	typedef void (*FStaticSetRenderFloat)( const char *name,float value );
	typedef float (*FStaticGetRenderFloat)( const char *name );
	
	typedef void (*FStaticSetRenderInt)( const char *name,int value );
	typedef int (*FStaticGetRender)( const char *name,int num_params,const Variant32 *params);
	typedef int (*FStaticGetRenderInt)( const char *name,int num_params,const Variant32 *params);
	typedef int (*FStaticGetEnum)( const char *name);
	typedef void (*FStaticProcessQueries)(int num,Query *q);

	typedef void (*FStaticCreateBoundedWaterObject) (unsigned int ID, float* dimension, float* location);
	typedef void (*FStaticRemoveBoundedWaterObject) (unsigned int ID);

	typedef void (*FStaticSetWaterFloat) (const char* name, unsigned int ID, float value);
	typedef void (*FStaticSetWaterInt) (const char* name, unsigned int ID, int value);
	typedef void (*FStaticSetWaterBool) (const char* name, unsigned int ID, bool value);
	typedef void (*FStaticSetWaterVector) (const char* name, int ID, float* value);
	typedef float (*FStaticGetWaterFloat) (const char* name, unsigned int ID);
	typedef int (*FStaticGetWaterInt) (const char* name, unsigned int ID);
	typedef bool (*FStaticGetWaterBool) (const char* name, unsigned int ID);
	typedef float* (*FStaticGetWaterVector) (const char* name, unsigned int ID);

	typedef void (*FStaticSetRenderString)( const char *name,const FString &value );
	typedef void (*FStaticGetRenderString)( const char *name ,char* value,int len);
	typedef bool (*FStaticTriggerAction)( const char *name );
	
	
	typedef void (*FStaticExportCloudLayerToGeometry)(const char *filenameUtf8,int index);

	typedef void (*FStaticSetKeyframeFloat)(unsigned,const char *name, float value);
	typedef float (*FStaticGetKeyframeFloat)(unsigned,const char *name);
	typedef void (*FStaticSetKeyframeInt)(unsigned,const char *name, int value);
	typedef int (*FStaticGetKeyframeInt)(unsigned,const char *name);

	typedef float (*FStaticGetRenderFloatAtPosition)(const char* name,const float *pos);

	// Using enums rather than char strings:
	typedef float (*FStaticGetFloatAtPosition)(long long enum_,const float *pos,int uid);
	typedef void (*FStaticGet)(long long enum_,Variant *variant);
	typedef void (*FStaticSet)(long long enum_,const Variant *variant);

	typedef bool (*FStaticFillColourTable)(unsigned uid,int x,int y,int z,float *target);

	FTrueSkyEditorCallback				TrueSkyEditorCallback;

	FStaticInitInterface				StaticInitInterface;
	FGetSimulVersion					GetSimulVersion;
	FStaticSetMemoryInterface			StaticSetMemoryInterface;
	FStaticShutDownInterface			StaticShutDownInterface;
	FStaticSetDebugOutputCallback		StaticSetDebugOutputCallback;
	FStaticSetGraphicsDevice			StaticSetGraphicsDevice;
	FStaticSetGraphicsDeviceAndContext	StaticSetGraphicsDeviceAndContext;	// Required for dx12 and switch
	FStaticPushPath						StaticPushPath;
	FStaticSetFileLoader				StaticSetFileLoader;
	FStaticGetOrAddView					StaticGetOrAddView;
	FStaticRenderFrame					StaticRenderFrame;
	FStaticRenderFrame2					StaticRenderFrame2;
	FStaticCopySkylight					StaticCopySkylight;
	FStaticCopySkylight2				StaticCopySkylight2;
	FStaticProbeSkylight				StaticProbeSkylight;
						
	FStaticRenderOverlays				StaticRenderOverlays;
	FStaticTick							StaticTick;
	FStaticOnDeviceChanged				StaticOnDeviceChanged;
	FStaticGetEnvironment				StaticGetEnvironment;
	FStaticSetSequence					StaticSetSequence;
	FStaticGetRenderInterfaceInstance	StaticGetRenderInterfaceInstance;
	FStaticSetPointLight				StaticSetPointLight;
	FStaticCloudPointQuery				StaticCloudPointQuery;
	FStaticLightingQuery				StaticLightingQuery;
	FStaticCloudLineQuery				StaticCloudLineQuery;
	FStaticSetRenderTexture				StaticSetRenderTexture;
	FStaticSetMatrix4x4					StaticSetMatrix4x4;
	FStaticSetRenderBool				StaticSetRenderBool;
	FStaticSetRender					StaticSetRender;
	FStaticGetRenderBool				StaticGetRenderBool;
	FStaticSetRenderFloat				StaticSetRenderFloat;
	FStaticGetRenderFloat				StaticGetRenderFloat;
	FStaticSetRenderInt					StaticSetRenderInt;
	FStaticGetRender					StaticGetRender;
	FStaticGetRenderInt					StaticGetRenderInt;
	FStaticGetEnum						StaticGetEnum;
	FStaticProcessQueries				StaticProcessQueries;
	FStaticSetRenderString				StaticSetRenderString;
	FStaticGetRenderString				StaticGetRenderString;
	FStaticTriggerAction				StaticTriggerAction;
	
	FStaticCreateBoundedWaterObject		StaticCreateBoundedWaterObject;
	FStaticRemoveBoundedWaterObject		StaticRemoveBoundedWaterObject;
	FStaticSetWaterFloat				StaticSetWaterFloat;
	FStaticSetWaterInt					StaticSetWaterInt;
	FStaticSetWaterBool					StaticSetWaterBool;
	FStaticSetWaterVector				StaticSetWaterVector;
	FStaticGetWaterFloat				StaticGetWaterFloat;
	FStaticGetWaterInt					StaticGetWaterInt;
	FStaticGetWaterBool					StaticGetWaterBool;
	FStaticGetWaterVector				StaticGetWaterVector;
	
	FStaticExportCloudLayerToGeometry	StaticExportCloudLayerToGeometry;

	FStaticSetKeyframeFloat				StaticSetKeyframeFloat;
	FStaticGetKeyframeFloat				StaticGetKeyframeFloat;
	FStaticSetKeyframeInt				StaticSetKeyframeInt;
	FStaticGetKeyframeInt				StaticGetKeyframeInt;
	FStaticGetRenderFloatAtPosition		StaticGetRenderFloatAtPosition;
	FStaticGetFloatAtPosition			StaticGetFloatAtPosition;
	FStaticGet							StaticGet;
	FStaticSet							StaticSet;
	FStaticGetLightningBolts			StaticGetLightningBolts;

	FStaticSpawnLightning				StaticSpawnLightning;

	FStaticFillColourTable				StaticFillColourTable;

	const TCHAR*			PathEnv;
	GraphicsDeviceType		CurrentRHIType;
	bool					RenderingEnabled;
	bool					RendererInitialized;

	bool					WaterRenderingEnabled;

	float					CachedDeltaSeconds;
	float					AutoSaveTimer;		// 0.0f == no auto-saving
	
	FRenderTarget			*cloudShadowRenderTarget;

	bool					actorPropertiesChanged;
	bool					haveEditor;
	bool					exportNext;
	char					exportFilenameUtf8[100];
	UTrueSkySequenceAsset *sequenceInUse;
	int LastFrameNumber, LastStereoFrameNumber;
	SimulVersion simulVersion;
 
	// Callback for when the settings were saved.
	bool HandleSettingsSaved()
	{
		UTrueSkySettings* Settings = GetMutableDefault<UTrueSkySettings>();
		bool ResaveSettings = false;
 
		// You can put any validation code in here and resave the settings in case an invalid
		// value has been entered
 
		if (ResaveSettings)
		{
			Settings->SaveConfig();
		}
 
		return true;
	}
};

IMPLEMENT_MODULE( FTrueSkyPlugin, TrueSkyPlugin )
FTrueSkyPlugin* FTrueSkyPlugin::Instance = nullptr;
static FString trueSkyPluginPath="../../Plugins/TrueSkyPlugin";

FTrueSkyPlugin::FTrueSkyPlugin()
	:GlobalOverlayFlag(true)
	,bIsInstancedStereoEnabled(false)
	,post_trans_registered(false)
	,AtmosphericsTexture(nullptr)
	,CloudTexture3D(nullptr)
	,PathEnv(nullptr)
	,CurrentRHIType(GraphicsDeviceUnknown)
	,RenderingEnabled(false)
	,RendererInitialized(false)
	,WaterRenderingEnabled(false)
	,CachedDeltaSeconds(0.0f)
	,AutoSaveTimer(0.0f)
	,cloudShadowRenderTarget(nullptr)
	,actorPropertiesChanged(false)
	,haveEditor(false)
	,exportNext(false)
	,sequenceInUse(nullptr)
	,LastFrameNumber(-1)
	,LastStereoFrameNumber(-1)
{
	simulVersion=ToSimulVersion(0,0,0);
	if(Instance)
		UE_LOG(TrueSky, Warning, TEXT("Instance is already set!"));
	if(!actorCrossThreadProperties)
		actorCrossThreadProperties=new ActorCrossThreadProperties;
	
	InitPaths();
	Instance = this;
#ifdef SHARED_FROM_THIS
TSharedRef< FTrueSkyPlugin,(ESPMode::Type)0 > ref=AsShared();
#endif
#if PLATFORM_WINDOWS || PLATFORM_UWP
	GlobalOverlayFlag=true;
#endif
	AutoSaveTimer = 0.0f;
	//we need to pass through real DeltaSecond; from our scene Actor?
	CachedDeltaSeconds = 0.0333f;
}

FTrueSkyPlugin::~FTrueSkyPlugin()
{
	Instance = nullptr;
	delete actorCrossThreadProperties;
	actorCrossThreadProperties=nullptr;
}

bool FTrueSkyPlugin::SupportsDynamicReloading()
{
	return true;
}
void FTrueSkyPlugin::SetCloudShadowRenderTarget(FRenderTarget *t)
{
	cloudShadowRenderTarget=t;
}

void FTrueSkyPlugin::RenderCloudShadow()
{
	if(!cloudShadowRenderTarget)
		return;
//	FTextureRenderTarget2DResource* res = (FTextureRenderTarget2DResource*)cloudShadowRenderTarget->Resource;
/*	FCanvas* Canvas = new FCanvas(cloudShadowRenderTarget, nullptr, nullptr);
	Canvas->Clear(FLinearColor::Blue);
	// Write text (no text is visible since the Canvas has no effect
	UFont* Font = GEngine->GetSmallFont();
	Canvas->DrawShadowedString(100, 100, TEXT("Test"), Font, FLinearColor::White);
	Canvas->Flush();
	delete Canvas;*/
}

void *FTrueSkyPlugin::GetRenderEnvironment()
{
	if( StaticGetEnvironment != nullptr )
	{
		return StaticGetEnvironment();
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to call StaticGetEnvironment before it has been set"));
		return nullptr;
	}
}

void FTrueSkyPlugin::LogCallback(const char *txt)
{
	if(!Instance )
		return;
	static FString fstr;
	fstr+=txt;
	int max_len=0;
	for(int i=0;i<fstr.Len();i++)
	{
		if(fstr[i]==L'\n'||i>1000)
		{
			fstr[i]=L' ';
			max_len=i+1;
			break;
		}
	}
	if(max_len==0)
		return;
	FString substr=fstr.Left(max_len);
	fstr=fstr.RightChop(max_len);
	if(substr.Contains("error"))
	{
		UE_LOG(TrueSky,Error,TEXT("%s"), *substr);
	}
	else if(substr.Contains("warning"))
	{
		UE_LOG(TrueSky,Warning,TEXT("%s"), *substr);
	}
	else
	{
		UE_LOG(TrueSky,Display,TEXT("%s"), *substr);
	}
}

void FTrueSkyPlugin::AdaptScaledMatrix(FMatrix &sMatrix)
{
	::AdaptScaledMatrix(sMatrix,actorCrossThreadProperties->MetresPerUnit,actorCrossThreadProperties->Transform.ToMatrixWithScale());
}

void FTrueSkyPlugin::AdaptViewMatrix(FMatrix &viewMatrix,bool editor_version)
{
	::AdaptViewMatrix(viewMatrix,actorCrossThreadProperties->MetresPerUnit,actorCrossThreadProperties->Transform.ToMatrixWithScale());
}

bool FTrueSkyPlugin::TriggerAction(const char *name)
{
	if( StaticTriggerAction != nullptr )
	{
		return StaticTriggerAction( name );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to TriggerAction before it has been set"));
	}
	return false;
}

bool FTrueSkyPlugin::TriggerAction(const FString &fname)
{
	std::string name=FStringToUtf8(fname);
	if( StaticTriggerAction != nullptr )
	{
		return StaticTriggerAction( name.c_str() );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to TriggerAction before it has been set"));
	}
	return false;
}
	
void FTrueSkyPlugin::SetRenderBool(const FString &fname, bool value)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRenderBool != nullptr )
	{
		StaticSetRenderBool(name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render bool before StaticSetRenderBool has been set"));
	}
}

bool FTrueSkyPlugin::GetRenderBool(const FString &fname) const
{
	std::string name=FStringToUtf8(fname);
	if( StaticGetRenderBool != nullptr )
	{
		return StaticGetRenderBool( name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render bool before StaticGetRenderBool has been set"));
	return false;
}

void FTrueSkyPlugin::SetRenderFloat(const FString &fname, float value)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRenderFloat != nullptr )
	{
		StaticSetRenderFloat( name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render float before StaticSetRenderFloat has been set"));
	}
}

float FTrueSkyPlugin::GetRenderFloat(const FString &fname) const
{
	std::string name=FStringToUtf8(fname);
	if( StaticGetRenderFloat != nullptr )
	{
		return StaticGetRenderFloat( name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render float before StaticGetRenderFloat has been set"));
	return 0.0f;
}

	

void FTrueSkyPlugin::SetRenderInt(const FString &fname, int value)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRenderInt != nullptr )
	{
		StaticSetRenderInt( name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render int before StaticSetRenderInt has been set"));
	}
}

	

void FTrueSkyPlugin::SetRender(const FString &fname, const TArray<FVariant> &params)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRender != nullptr )
	{
		Variant32 varlist[6];
		int num_params=params.Num();
		if(num_params>5)
		{
			UE_LOG_ONCE(TrueSky, Warning, TEXT("Too many parameters."));
			return ;
		}
		for(int i=0;i<num_params;i++)
		{
			if(params[i].GetType()==EVariantTypes::Int32)
				varlist[i].intVal=params[i].GetValue<int32>();
			else if(params[i].GetType()==EVariantTypes::Int32)
				varlist[i].intVal=params[i].GetValue<int32>();
			else if(params[i].GetType()==EVariantTypes::Float)
				varlist[i].floatVal=params[i].GetValue<float>();
			else if(params[i].GetType()==EVariantTypes::Double)
				varlist[i].floatVal=params[i].GetValue<double>();
			else
			{
				UE_LOG_ONCE(TrueSky, Warning, TEXT("Unsupported variant type."));
				return ;
			}
		}
		varlist[num_params].intVal=0;
		StaticSetRender( name.c_str(), num_params,varlist);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render int before StaticSetRenderInt has been set"));
	}
}
int FTrueSkyPlugin::GetRenderInt(const FString &fname) const
{
	TArray<FVariant> params;
	return GetRenderInt(fname,params);
}

int64 FTrueSkyPlugin::GetEnum(const FString &fname) const
{
	std::string name=FStringToUtf8(fname);
	if(StaticGetEnum)
		return StaticGetEnum(name.c_str());
	else
		return 0;
}

int FTrueSkyPlugin::GetRenderInt(const FString &fname,const TArray<FVariant> &params) const
{
	std::string name=FStringToUtf8(fname);
	if( StaticGetRenderInt != nullptr )
	{
		Variant32 varlist[6];
		int num_params=params.Num();
		if(num_params>5)
		{
			UE_LOG_ONCE(TrueSky, Warning, TEXT("Too many parameters."));
			return 0;
		}
		for(int i=0;i<num_params;i++)
		{
			if(params[i].GetType()==EVariantTypes::Int32)
				varlist[i].intVal=params[i].GetValue<int32>();
			else if(params[i].GetType()==EVariantTypes::Int32)
				varlist[i].intVal=params[i].GetValue<int32>();
			else if(params[i].GetType()==EVariantTypes::Float)
				varlist[i].floatVal=params[i].GetValue<float>();
			else if(params[i].GetType()==EVariantTypes::Double)
				varlist[i].floatVal=params[i].GetValue<double>();
			else
			{
				UE_LOG_ONCE(TrueSky, Warning, TEXT("Unsupported variant type."));
				return 0;
			}
		}
		varlist[num_params].intVal=0;
		return StaticGetRenderInt( name.c_str(),num_params,varlist);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render float before StaticGetRenderInt has been set"));
	return 0;
}

void FTrueSkyPlugin::SetRenderString(const FString &fname, const FString &value)
{
	if( StaticSetRenderString != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		StaticSetRenderString( name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render string before StaticSetRenderString has been set"));
	}
}

FString FTrueSkyPlugin::GetRenderString(const FString &fname) const
{
	if( StaticGetRenderString != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		static char txt[14500];
		StaticGetRenderString( name.c_str(),txt,14500);
		return FString(txt);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render string before StaticGetRenderString has been set"));
	return "";
}

void FTrueSkyPlugin::CreateBoundedWaterObject(unsigned int ID, float* dimension, float* location)
{
	if (StaticCreateBoundedWaterObject != nullptr)
	{
		// RK: Disable for now due to initialization order problem:
		StaticCreateBoundedWaterObject(ID, dimension, location);
		waterActorsCrossThreadProperties[ID]->BoundedObjectCreated = true;
	}
	else
	{
		//UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to create water object before StaticCreateBoundedWaterObject has been set"));
	}
}

void FTrueSkyPlugin::RemoveBoundedWaterObject(unsigned int ID)
{
	if (StaticRemoveBoundedWaterObject != nullptr)
	{
		StaticRemoveBoundedWaterObject(ID);
		waterActorsCrossThreadProperties[ID]->BoundedObjectCreated = false;
	}
	else
	{
		//UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to remove water object before StaticRemoveBoundedWaterObject has been set"));
	}
} 

void FTrueSkyPlugin::SetWaterFloat(const FString &fname, unsigned int ID, float value)
{	
	if (StaticSetWaterFloat != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterFloat(name.c_str(), ID, value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water float before StaticSetWaterFloat has been set"));
	}
}

void FTrueSkyPlugin::SetWaterInt(const FString &fname, unsigned int ID, int value)
{
	if (StaticSetWaterInt != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterInt(name.c_str(), ID, value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water float before StaticSetWaterFloat has been set"));
	}
}

void FTrueSkyPlugin::SetWaterBool(const FString &fname, unsigned int ID, bool value)
{
	if (StaticSetWaterBool != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterBool(name.c_str(), ID, value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water bool before StaticSetWaterBool has been set"));
	}
}

void FTrueSkyPlugin::SetWaterVector(const FString &fname, int ID, FVector value)
{
	if (StaticSetWaterVector != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterVector(name.c_str(), ID, (float*)&value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water vector before StaticSetWaterVector has been set"));
	}
}

float FTrueSkyPlugin::GetWaterFloat(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	if (StaticGetWaterFloat != nullptr)
	{
		return StaticGetWaterFloat(name.c_str(), ID);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water float before StaticGetWaterFloat has been set"));
	return 0.0f;
}

int FTrueSkyPlugin::GetWaterInt(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	if (StaticGetWaterInt != nullptr)
	{
		return StaticGetWaterInt(name.c_str(), ID);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water int before StaticGetWaterInt has been set"));
	return 0;
}

bool FTrueSkyPlugin::GetWaterBool(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	if (StaticGetWaterFloat != nullptr)
	{
		return StaticGetWaterBool(name.c_str(), ID);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water bool before StaticGetWaterBool has been set"));
	return false;
}

float* FTrueSkyPlugin::GetWaterVector(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	if (StaticGetWaterVector != nullptr)
	{
		return StaticGetWaterVector(name.c_str(), ID);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water vector before StaticGetWaterVector has been set"));
	return NULL;
}


void FTrueSkyPlugin::SetKeyframeFloat(unsigned uid,const FString &fname, float value)
{
	if( StaticSetKeyframeFloat != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		StaticSetKeyframeFloat(uid,name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe float before StaticSetKeyframeFloat has been set"));
	}
}

float FTrueSkyPlugin::GetKeyframeFloat(unsigned uid,const FString &fname) const
{
	if( StaticGetKeyframeFloat != nullptr )
	{
	std::string name=FStringToUtf8(fname);
		return StaticGetKeyframeFloat( uid,name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get Keyframe float before StaticGetKeyframeFloat has been set"));
	return 0.0f;
}

float FTrueSkyPlugin::GetRenderFloatAtPosition(const FString &fname,FVector pos) const
{
	if( StaticGetRenderFloatAtPosition != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
		pos=UEToTrueSkyPosition(A->Transform,A->MetresPerUnit,pos);
		return StaticGetRenderFloatAtPosition( name.c_str(),(const float*)&pos);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get float at position before StaticGetRenderFloatAtPosition has been set"));
	return 0.0f;
}

float FTrueSkyPlugin::GetFloatAtPosition(int64 Enum,FVector pos,int32 uid) const
{
	if( StaticGetFloatAtPosition != nullptr )
	{
		ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
		pos=UEToTrueSkyPosition(A->Transform,A->MetresPerUnit,pos);
		return StaticGetFloatAtPosition( Enum,(const float*)&pos,uid);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get float at position before StaticGetFloatAtPosition has been set"));
	return 0.0f;
}

float FTrueSkyPlugin::GetFloat(int64 Enum) const
{
	if( StaticGet != nullptr )
	{
		float f;
		StaticGet( Enum,(Variant*)&f);
		return f;
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get float at position before StaticGetFloatAtPosition has been set"));
	return 0.0f;
}

FVector FTrueSkyPlugin::GetVector(int64 Enum) const
{
	FVector v;
	if( StaticGet != nullptr )
	{
		StaticGet( Enum,(Variant*)&v);
		return v;
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get a vector before StaticGet has been set"));
	return v;
}

void FTrueSkyPlugin::SetVector(int64 Enum,FVector v) const
{
	if( StaticSet != nullptr )
	{
		StaticSet( Enum,(const Variant*)&v);
	}
	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set a vector before SetVector has been set"));
}

void FTrueSkyPlugin::SetInt( ETrueSkyPropertiesInt Property, int32 Value )
{
	const int32 arrElements = ATrueSkySequenceActor::TrueSkyPropertyIntMaps.Num();
	if (arrElements && ATrueSkySequenceActor::TSPropertiesIntIdx <= arrElements - 1)
	{
		auto p = ATrueSkySequenceActor::TrueSkyPropertyIntMaps[ATrueSkySequenceActor::TSPropertiesIntIdx].Find((int64)Property);
		if (p->TrueSkyEnum>0)
		{
			StaticSet(p->TrueSkyEnum, (Variant*)&Value);
		}
	}
	else
	{
		UE_LOG(TrueSky, Warning, TEXT("Trying to set an int property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetVector( ETrueSkyPropertiesVector Property, FVector Value )
{
	const int32 arrElements = ATrueSkySequenceActor::TrueSkyPropertyVectorMaps.Num();
	if (arrElements && ATrueSkySequenceActor::TSPropertiesVectorIdx <= arrElements - 1)
	{
		auto p = ATrueSkySequenceActor::TrueSkyPropertyVectorMaps[ATrueSkySequenceActor::TSPropertiesVectorIdx].Find((int64)Property);
		if (p->TrueSkyEnum>0)
		{
			StaticSet(p->TrueSkyEnum, (Variant*)&Value);
		}
	}
	else
	{
		UE_LOG(TrueSky, Warning, TEXT("Trying to set a vector property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetFloat( ETrueSkyPropertiesFloat Property, float Value )
{
	const int32 arrElements = ATrueSkySequenceActor::TrueSkyPropertyFloatMaps.Num();
	if (arrElements && ATrueSkySequenceActor::TSPropertiesFloatIdx <= arrElements - 1)
	{
		auto p = ATrueSkySequenceActor::TrueSkyPropertyFloatMaps[ATrueSkySequenceActor::TSPropertiesFloatIdx].Find((int64)Property);
		if (p->TrueSkyEnum > 0)
		{
			StaticSet(p->TrueSkyEnum, (Variant*)&Value);
		}
	}
	else
	{
		UE_LOG(TrueSky, Warning, TEXT("Trying to set a float property but the index is out of range or the array is empty!"));
	}
}
void FTrueSkyPlugin::SetKeyframeInt(unsigned uid,const FString &fname, int value)
{
	if( StaticSetKeyframeInt != nullptr )
	{
	std::string name=FStringToUtf8(fname);
		StaticSetKeyframeInt( uid,name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe int before StaticSetKeyframeInt has been set"));
	}
}

int FTrueSkyPlugin::GetKeyframeInt(unsigned uid,const FString &fname) const
{
	if( StaticGetKeyframeInt != nullptr )
	{
	std::string name=FStringToUtf8(fname);
		return StaticGetKeyframeInt( uid,name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get Keyframe float before StaticGetKeyframeInt has been set"));
	return 0;
}

/** Tickable object interface */
void FTrueSkyTickable::Tick( float DeltaTime )
{
	//if(FTrueSkyPlugin::Instance)
	//	FTrueSkyPlugin::Instance->Tick(DeltaTime);
}

bool FTrueSkyTickable::IsTickable() const
{
	return true;
}

TStatId FTrueSkyTickable::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTrueSkyTickable, STATGROUP_Tickables);
}


void FTrueSkyPlugin::StartupModule()
{
	if(FModuleManager::Get().IsModuleLoaded("MainFrame") )
		haveEditor=true;
	GetRendererModule().RegisterPostOpaqueRenderDelegate( FRenderDelegate::CreateRaw(this, &FTrueSkyPlugin::DelegatedRenderFrame) );
#if !SIMUL_BINARY_PLUGIN
	GetRendererModule().RegisterPostTranslucentRenderDelegate( FRenderDelegate::CreateRaw(this, &FTrueSkyPlugin::DelegatedRenderPostTranslucent) );
	post_trans_registered = true;
#endif
#if !UE_BUILD_SHIPPING
	GetRendererModule().RegisterOverlayRenderDelegate( FRenderDelegate::CreateRaw(this, &FTrueSkyPlugin::DelegatedRenderOverlays) );
#endif
#if WITH_EDITOR
	// register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "trueSKY",
			LOCTEXT("TrueSkySettingsName", "trueSKY"),
			LOCTEXT("TrueSkySettingsDescription", "Configure the trueSKY plug-in."),
			GetMutableDefault<UTrueSkySettings>()
		);

		// Register the save handler to your settings, you might want to use it to
		// validate those or just act to settings changes.
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FTrueSkyPlugin::HandleSettingsSaved);
		}
	}
#endif //WITH_EDITOR

	RenderingEnabled					=false;
	RendererInitialized					=false;
	WaterRenderingEnabled				=false;
	TrueSkyEditorCallback				=nullptr;
	StaticInitInterface					=nullptr;
	GetSimulVersion						=nullptr;
	StaticSetMemoryInterface			=nullptr;
	StaticShutDownInterface				=nullptr;
	StaticSetDebugOutputCallback		=nullptr;
	StaticSetGraphicsDevice				=nullptr;
	StaticSetGraphicsDeviceAndContext	=nullptr;
	StaticPushPath						=nullptr;
	StaticSetFileLoader					=nullptr;
	StaticGetOrAddView					=nullptr;
	StaticRenderFrame					=nullptr;
	StaticRenderFrame2					=nullptr;
	StaticCopySkylight					=nullptr;
	StaticCopySkylight2					=nullptr;
	StaticProbeSkylight					=nullptr;
	StaticRenderOverlays				=nullptr;
	StaticTick							=nullptr;
	StaticOnDeviceChanged				=nullptr;
	StaticGetEnvironment				=nullptr;
	StaticSetSequence					=nullptr;

	StaticGetRenderInterfaceInstance	=nullptr;
	StaticSetPointLight					=nullptr;
	StaticCloudPointQuery				=nullptr;
	StaticLightingQuery					=nullptr;
	StaticCloudLineQuery				=nullptr;
	StaticSetRenderTexture				=nullptr;
	StaticSetMatrix4x4					=nullptr;
	StaticSetRenderBool					=nullptr;
	StaticSetRender						=nullptr;
	StaticGetRenderBool					=nullptr;
	StaticTriggerAction					=nullptr;

	StaticSetRenderFloat				=nullptr;
	StaticGetRenderFloat				=nullptr;
	StaticSetRenderInt					=nullptr;
	StaticGetRender						=nullptr;
	
	StaticSetRenderString				=nullptr;
	StaticGetRenderString				=nullptr;
										
	StaticCreateBoundedWaterObject		=nullptr;
	StaticRemoveBoundedWaterObject		=nullptr;

	StaticSetWaterFloat					=nullptr;
	StaticSetWaterInt					=nullptr;
	StaticSetWaterBool					=nullptr;
	StaticSetWaterVector				=nullptr;
	StaticGetWaterFloat					=nullptr;
	StaticGetWaterInt					=nullptr;
	StaticGetWaterBool					=nullptr;
	StaticGetWaterVector				=nullptr;
										
	StaticExportCloudLayerToGeometry	=nullptr;
										
	StaticSetKeyframeFloat				=nullptr;
	StaticGetKeyframeFloat				=nullptr;
	StaticSetKeyframeInt				=nullptr;
	StaticGetKeyframeInt				=nullptr;
	StaticGetLightningBolts				=nullptr;
	StaticSpawnLightning				=nullptr;

	StaticGetRenderFloatAtPosition		=nullptr;

	StaticGetFloatAtPosition			=nullptr;
	StaticGet							=nullptr;
	StaticSet							=nullptr;
	StaticGetEnum						=nullptr;
	StaticProcessQueries				=nullptr;
		
	StaticFillColourTable				=nullptr;
	PathEnv = nullptr;

	// Perform the initialization earlier on the game thread
	EnsureRendererIsInitialized();
}
//FGraphEventRef tsFence;
#if PLATFORM_PS4
#endif

void FTrueSkyPlugin::SetRenderingEnabled( bool Enabled )
{
	RenderingEnabled = Enabled;
}

void FTrueSkyPlugin::SetWaterRenderingEnabled( bool Enabled)
{
	WaterRenderingEnabled = Enabled;
}

bool FTrueSkyPlugin::GetWaterRenderingEnabled()
{
	return WaterRenderingEnabled;
}

struct FRHIRenderOverlaysCommand : public FRHICommand<FRHIRenderOverlaysCommand>
{
	FRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	int FrameNumber;
	FORCEINLINE_DEBUGGABLE FRHIRenderOverlaysCommand(FRenderDelegateParameters p,
												FTrueSkyPlugin *d,uint64_t u,int f)
												:RenderParameters(p)
												,TrueSkyPlugin(d)
												,uid(u)
												,FrameNumber(f)
	{
	}
    void Execute(FRHICommandListBase& CmdList)
	{
		TrueSkyPlugin->RenderOverlays(uid,RenderParameters, FrameNumber);
	}
};

void FTrueSkyPlugin::DelegatedRenderOverlays(FRenderDelegateParameters& RenderParameters)
{	
	if(!GlobalOverlayFlag)
		return;
#if !SIMUL_BINARY_PLUGIN
	if (RenderParameters.RenderTargetTexture && ((RenderParameters.RenderTargetTexture->GetFlags()&TexCreate_RenderTargetable) != TexCreate_RenderTargetable))
		return;
#endif
	FSceneView *View=(FSceneView*)(RenderParameters.Uid);
	Viewport v;
	v.x=RenderParameters.ViewportRect.Min.X;
	v.y=RenderParameters.ViewportRect.Min.Y;
	v.w=RenderParameters.ViewportRect.Width();
	v.h=RenderParameters.ViewportRect.Height();
	v.zmin = 0.0f;
	v.zmax = 1.0f;
	// We really don't want to be drawing on preview views etc. so set an arbitrary minimum view size.
	const UTrueSkySettings *TrueSkySettings = GetDefault<UTrueSkySettings>();
	int min_view_size=TrueSkySettings->MinimumViewSize;
	if(v.w<min_view_size&&v.h<min_view_size)
		return;
	FSceneViewState* ViewState = (FSceneViewState*)View->State;
	if(!ViewState)
		return;
	if(View->Family)
	{
		switch(View->Family->SceneCaptureSource)
		{
		case SCS_SceneColorHDR:		
		case SCS_SceneColorHDRNoAlpha:
		case SCS_FinalColorLDR:
		case SCS_BaseColor:
			break;
		default:
			return;
		};
	}
	uint64_t uid=10131101303;
	if(ViewState)
		uid=ViewState->UniqueID;
	// Not View->Family->FrameNumber but GFrameNumber
	if (RenderParameters.RHICmdList->Bypass())
	{
		FRHIRenderOverlaysCommand Command(RenderParameters,this,uid,GFrameNumber);
		Command.Execute(*RenderParameters.RHICmdList);			
	}	
	else
	{
	    new (RenderParameters.RHICmdList->AllocCommand<FRHIRenderOverlaysCommand>()) FRHIRenderOverlaysCommand(RenderParameters,this,uid,GFrameNumber);
	}
}

void FTrueSkyPlugin::RenderOverlays(uint64_t uid,FRenderDelegateParameters& RenderParameters,int FrameNumber)
{	
#if TRUESKY_PLATFORM_SUPPORTED
	if(!GlobalOverlayFlag)
		return;
	if(!RenderParameters.ViewportRect.Width()||!RenderParameters.ViewportRect.Height())
		return;
	UpdateFromActor();
	//if(!RenderingEnabled )
	//	return;
	if(!RendererInitialized )
		return;
	//if( RenderingEnabled )
	{
		StoreUEState(RenderParameters);
		//StaticTick( 0 );
		Viewport viewports[3];
		viewports[0].x=RenderParameters.ViewportRect.Min.X;
		viewports[0].y=RenderParameters.ViewportRect.Min.Y;
		viewports[0].w=RenderParameters.ViewportRect.Width();
		viewports[0].h=RenderParameters.ViewportRect.Height();
		viewports[0].zmin = 0.0f;
		viewports[0].zmax = 1.0f;
		// VR Viewports:
		for(int i=1;i<3;i++)
		{
			viewports[i].x=(i==2?RenderParameters.ViewportRect.Width()/2:0);
			viewports[i].y=RenderParameters.ViewportRect.Min.Y;
			viewports[i].w=RenderParameters.ViewportRect.Width()/2;
			viewports[i].h=RenderParameters.ViewportRect.Height();
			viewports[i].zmin = 0.0f;
			viewports[i].zmax = 1.0f;
		}

		void *device=GetPlatformDevice();
		void* pContext		= GetPlatformContext(RenderParameters, CurrentRHIType);
		void* depthTexture	= GetPlatformTexturePtr(RenderParameters.DepthTexture, CurrentRHIType);
		void* colourTarget	= GetColourTarget(RenderParameters, CurrentRHIType);

		IRHICommandContext *CommandListContext=(IRHICommandContext*)(&RenderParameters.RHICmdList->GetContext());

        int view_id = StaticGetOrAddView((void*)uid);		// RVK: really need a unique view ident to pass here..
		static int overlay_id=0;
		FMatrix projMatrix = RenderParameters.ProjMatrix;
		projMatrix.M[2][3]	*=-1.0f;
		projMatrix.M[3][2]	*= actorCrossThreadProperties->MetresPerUnit;
		FMatrix viewMatrix=RenderParameters.ViewMatrix;
		// We want the transform FROM worldspace TO trueskyspace
		AdaptViewMatrix(viewMatrix);

#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
		if (CurrentRHIType == GraphicsDeviceD3D12)
		{
			OverlaysStoreStateD3D12(&RenderParameters.RHICmdList->GetContext(), RenderParameters.RenderTargetTexture,RenderParameters.DepthTexture);
		}
#endif

		StaticRenderOverlays(device,pContext, depthTexture,&(viewMatrix.M[0][0]),&(projMatrix.M[0][0]),view_id,colourTarget,viewports);

#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
		if (CurrentRHIType == GraphicsDeviceD3D12)
		{
			OverlaysRestoreStateD3D12(&RenderParameters.RHICmdList->GetContext(), RenderParameters.RenderTargetTexture, RenderParameters.DepthTexture);
		}
#endif

		RestoreUEState(RenderParameters, CurrentRHIType);
	}
#endif
}

struct FRHIPostOpaqueCommand: public FRHICommand<FRHIPostOpaqueCommand>
{
	FRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	EStereoscopicPass StereoPass;
	bool bMultiResEnabled;
	bool bIncludePostTranslucent;
	int FrameNumber;
	FORCEINLINE_DEBUGGABLE FRHIPostOpaqueCommand(FRenderDelegateParameters p,
		FTrueSkyPlugin *d,uint64_t u,EStereoscopicPass s,bool m,int f,bool post_trans)
		:RenderParameters(p)
		,TrueSkyPlugin(d)
		,uid(u)
		,StereoPass(s)
		,bMultiResEnabled(m)
		,bIncludePostTranslucent(post_trans)
		,FrameNumber(f)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		if(TrueSkyPlugin)
		{
			TrueSkyPlugin->RenderFrame(uid,RenderParameters,StereoPass,bMultiResEnabled,FrameNumber);
			if(bIncludePostTranslucent)
				TrueSkyPlugin->RenderPostTranslucent(uid,RenderParameters,StereoPass,bMultiResEnabled,FrameNumber);
		}
	}
};

struct FRHIPostTranslucentCommand: public FRHICommand<FRHIPostTranslucentCommand>
{
	FRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	EStereoscopicPass StereoPass;
	bool bMultiResEnabled;
	int FrameNumber;
	FORCEINLINE_DEBUGGABLE FRHIPostTranslucentCommand(FRenderDelegateParameters p,
		FTrueSkyPlugin *d,uint64_t u,EStereoscopicPass s,bool m,int f)
		:RenderParameters(p)
		,TrueSkyPlugin(d)
		,uid(u)
		,StereoPass(s)
		,bMultiResEnabled(m)
		,FrameNumber(f)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		if(TrueSkyPlugin)
			TrueSkyPlugin->RenderPostTranslucent(uid,RenderParameters,StereoPass,bMultiResEnabled,FrameNumber);
	}
};

void FTrueSkyPlugin::DelegatedRenderPostTranslucent(FRenderDelegateParameters& RenderParameters)
{
#if !SIMUL_BINARY_PLUGIN
	if (RenderParameters.RenderTargetTexture && ((RenderParameters.RenderTargetTexture->GetFlags()&TexCreate_RenderTargetable) != TexCreate_RenderTargetable))
		return;
#endif
	FSceneView *View=(FSceneView*)(RenderParameters.Uid);
	bool bMultiResEnabled =false;
#ifdef NV_MULTIRES
	bMultiResEnabled=View->bVRProjectEnabled;
#endif
	EStereoscopicPass StereoPass=View->StereoPass;
	Viewport v;
	v.x=RenderParameters.ViewportRect.Min.X;
	v.y=RenderParameters.ViewportRect.Min.Y;
	v.w=RenderParameters.ViewportRect.Width();
	v.h=RenderParameters.ViewportRect.Height();	// We really don't want to be drawing on preview views etc. so set an arbitrary minimum view size.
	v.zmin = 0.0f;
	v.zmax = 1.0f;
	const UTrueSkySettings *TrueSkySettings = GetDefault<UTrueSkySettings>();
	int min_view_size=TrueSkySettings->MinimumViewSizeForTranslucent;
	
	if(v.w<min_view_size&&v.h<min_view_size)
		return;
	FSceneViewState* ViewState = (FSceneViewState*)View->State;
	if(TrueSkySettings->RenderOnlyIdentifiedViews&&!ViewState)
		return;
	if(View->Family)
	{
		switch(View->Family->SceneCaptureSource)
		{
		case SCS_SceneColorHDR:		
		case SCS_SceneColorHDRNoAlpha:
		case SCS_FinalColorLDR:
		case SCS_BaseColor:
			break;
		default:
			return;
		};
	}
	uint64_t uid=10131101303;
	if(ViewState)
		uid=ViewState->UniqueID;

#if !SIMUL_BINARY_PLUGIN
	if(RenderParameters.SceneContext->IsSeparateTranslucencyPass())
	{
		FIntPoint TranslucencySize(RenderParameters.ViewportRect.Width(),RenderParameters.ViewportRect.Height());
		float TranslucencyScale;
		FIntPoint sz=RenderParameters.ViewportRect.Size();
		RenderParameters.SceneContext->GetSeparateTranslucencyDimensions(sz, TranslucencyScale);
		if(TranslucencyScale!=1.0f)
		{
			TranslucencySize.X*=TranslucencyScale;
			TranslucencySize.Y*=TranslucencyScale;
			RenderParameters.ViewportRect.Max= RenderParameters.ViewportRect.Min + TranslucencySize;
		}
	}
#endif

	if(RenderParameters.RHICmdList->Bypass())
	{
		FRHIPostTranslucentCommand Command(RenderParameters,this,uid,StereoPass,bMultiResEnabled,GFrameNumber);
		Command.Execute(*RenderParameters.RHICmdList);
	}
	else
	{
	    new (RenderParameters.RHICmdList->AllocCommand<FRHIPostTranslucentCommand>()) FRHIPostTranslucentCommand(RenderParameters,this,uid,StereoPass,bMultiResEnabled,GFrameNumber);
	}
}

void FTrueSkyPlugin::RenderPostTranslucent(uint64_t uid,FRenderDelegateParameters& RenderParameters
														,EStereoscopicPass StereoPass
														,bool bMultiResEnabled
														,int FrameNumber)
{
#if TRUESKY_PLATFORM_SUPPORTED
	if(RenderParameters.Uid==0)
	{
		if(StaticShutDownInterface)
			StaticShutDownInterface();
		sequenceInUse=nullptr;
		return;
	}
	if(!actorCrossThreadProperties->Visible)
		return;
	ERRNO_CLEAR
	//check(IsInRenderingThread());
	if(!RenderParameters.ViewportRect.Width()||!RenderParameters.ViewportRect.Height())
		return;
	if(!RenderingEnabled)
		return;
	if(!StaticGetOrAddView)
		return;
	if(!actorCrossThreadProperties->activeSequence)
		return;
	if (!actorCrossThreadProperties->activeSequence->IsValidLowLevel())
		return;

	QUICK_SCOPE_CYCLE_COUNTER(TrueSkyPostTrans);

	void *device=GetPlatformDevice();
	void *pContext=GetPlatformContext(RenderParameters, CurrentRHIType);
	PluginStyle style=UNREAL_STYLE;
	FSceneView *View=(FSceneView*)(RenderParameters.Uid);

	if(StereoPass==eSSP_LEFT_EYE)
		uid=29435;
	else if(StereoPass==eSSP_RIGHT_EYE)
		uid=29435;
    int view_id = StaticGetOrAddView((void*)uid);		// RVK: really need a unique view ident to pass here..

	FMatrix viewMatrix=RenderParameters.ViewMatrix;
	// We want the transform FROM worldspace TO trueskyspace
	AdaptViewMatrix(viewMatrix);
	FMatrix projMatrix	=RenderParameters.ProjMatrix;
	AdaptProjectionMatrix(projMatrix, actorCrossThreadProperties->MetresPerUnit);

	float exposure			= actorCrossThreadProperties->Brightness;
	static float g			= 1.0f;
	float gamma				= g;//actorCrossThreadProperties->Gamma;
	void* depthTexture		= GetPlatformTexturePtr(RenderParameters.DepthTexture, CurrentRHIType);
	void* colourTarget		= GetColourTarget(RenderParameters,CurrentRHIType);
#if !SIMUL_BINARY_PLUGIN
	void* colourTargetTexture = GetPlatformTexturePtr(RenderParameters.RenderTargetTexture, CurrentRHIType);
#else
	void* colourTargetTexture=nullptr;
#endif

	int4 depthViewport;
	depthViewport.x=RenderParameters.ViewportRect.Min.X;
	depthViewport.y=RenderParameters.ViewportRect.Min.Y;
	depthViewport.z=RenderParameters.ViewportRect.Width();
	depthViewport.w=RenderParameters.ViewportRect.Height();
	
	Viewport viewports[3];
	viewports[0].x=RenderParameters.ViewportRect.Min.X;
	viewports[0].y=RenderParameters.ViewportRect.Min.Y;
	viewports[0].w=RenderParameters.ViewportRect.Width();
	viewports[0].h=RenderParameters.ViewportRect.Height();
	viewports[0].zmin = 0.0f;
	viewports[0].zmax = 1.0f;
	// VR Viewports:
	for(int i=1;i<3;i++)
	{
		viewports[i].x=(i==2?RenderParameters.ViewportRect.Width()/2:0);
		viewports[i].y=RenderParameters.ViewportRect.Min.Y;
		viewports[i].w=RenderParameters.ViewportRect.Width()/2;
		viewports[i].h=RenderParameters.ViewportRect.Height();
		viewports[i].zmin = 0.0f;
		viewports[i].zmax = 1.0f;
	}
	
	// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
	TArray<FVector2D>  MultiResConstants;
#ifdef NV_MULTIRES
	if (bMultiResEnabled)
	{
		SetMultiResConstants(MultiResConstants,View);
	}
#endif
	// NVCHANGE_END: TrueSky + VR MultiRes Support
	style=style|POST_TRANSLUCENT;
	// We need a real time timestamp for the RHI thread.
	if(actorCrossThreadProperties->Playing)
	{
		StaticTriggerAction("CalcRealTime"); 
	}
	StoreUEState(RenderParameters);

#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		PostTranslucentStoreStateD3D12(&RenderParameters.RHICmdList->GetContext(),RenderParameters.RenderTargetTexture, RenderParameters.DepthTexture);
	}
#endif
	if (StaticRenderFrame2)
	{
		RenderFrameStruct frameStruct;
		frameStruct.colourTarget = colourTarget;
		frameStruct.device = device;
		frameStruct.pContext = pContext;
		frameStruct.view_id = view_id;
		frameStruct.viewMatrix4x4 = &(viewMatrix.M[0][0]);
		frameStruct.projMatrix4x4 = &(projMatrix.M[0][0]);
		frameStruct.depthTexture = depthTexture;
		frameStruct.colourTarget = colourTarget;
		frameStruct.colourTargetTexture = colourTargetTexture;
		frameStruct.isColourTargetTexture = true;
		frameStruct.depthViewport = depthViewport;
		frameStruct.targetViewport = viewports;
		frameStruct.style = style;
		frameStruct.exposure = exposure;
		frameStruct.gamma = gamma;
		frameStruct.framenumber = FrameNumber;
		frameStruct.nvMultiResConstants = (bMultiResEnabled ? (const float*)(MultiResConstants.GetData()) : nullptr);

		StaticRenderFrame2(&frameStruct);

	}
	else {
		StaticRenderFrame(device, pContext, view_id, &(viewMatrix.M[0][0]), &(projMatrix.M[0][0])
			, depthTexture, colourTarget
			, depthViewport
			, viewports
			, style
			, exposure
			, gamma
			, FrameNumber
			// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
			, (bMultiResEnabled ? (const float*)(MultiResConstants.GetData()) : nullptr)
			// NVCHANGE_END: TrueSky + VR MultiRes Support
		);
	}
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		PostTranslucentRestoreStateD3D12(&RenderParameters.RHICmdList->GetContext(), RenderParameters.RenderTargetTexture, RenderParameters.DepthTexture);
	}
#endif
	RestoreUEState(RenderParameters, CurrentRHIType);

#endif
}

void FTrueSkyPlugin::DelegatedRenderFrame(FRenderDelegateParameters& RenderParameters)
{
#if !SIMUL_BINARY_PLUGIN
	if (RenderParameters.RenderTargetTexture && ((RenderParameters.RenderTargetTexture->GetFlags()&TexCreate_RenderTargetable) != TexCreate_RenderTargetable))
		return;
#endif
	FSceneView *View=(FSceneView*)(RenderParameters.Uid);
	Viewport v;
	v.x=RenderParameters.ViewportRect.Min.X;
	v.y=RenderParameters.ViewportRect.Min.Y;
	v.w=RenderParameters.ViewportRect.Width();
	v.h=RenderParameters.ViewportRect.Height();
	v.zmin = 0.0f;
	v.zmax = 1.0f;
	bool bMultiResEnabled=false;
#ifdef NV_MULTIRES
	bMultiResEnabled=View->bVRProjectEnabled;
#endif
	const UTrueSkySettings *TrueSkySettings = GetDefault<UTrueSkySettings>();
	int ViewSize = std::min(v.h, v.w);
	if (ViewSize < TrueSkySettings->MinimumViewSize)
		return;
	if(View->Family)
	{
		switch(View->Family->SceneCaptureSource)
		{
		case SCS_SceneColorHDR:		
		case SCS_SceneColorHDRNoAlpha:
		case SCS_FinalColorLDR:
		case SCS_BaseColor:
			break;
		default:
			return;
		};
	}
	bool bIncludePostTranslucent=(!post_trans_registered)&&(ViewSize>=TrueSkySettings->MinimumViewSizeForTranslucent);
	bIsInstancedStereoEnabled=View->bIsInstancedStereoEnabled;
	EStereoscopicPass StereoPass=View->StereoPass;
	FSceneViewState* ViewState = (FSceneViewState*)View->State;
	if(TrueSkySettings->RenderOnlyIdentifiedViews&&!ViewState)
		return;
//	SCOPED_DRAW_EVENT(*RenderParameters.RHICmdList, DelegatedRenderFrame);
//	SCOPED_GPU_STAT(RenderParameters.RHICmdList, Stat_GPU_DelegatedRenderFrame);
	uint64_t uid=(uint64_t)(View);//10131101303;
	static uint64_t default_uid=10131101303;
	if(default_uid)
		uid=default_uid;
	if(ViewState)
		uid=ViewState->UniqueID;
	// Load the dll and initialize. This should be done on the Game thread.
	//if(!EnsureRendererIsInitialized())
		//return;

	if(RenderParameters.RHICmdList->Bypass())
	{
		FRHIPostOpaqueCommand Command(RenderParameters,this,uid,StereoPass,bMultiResEnabled,GFrameNumber,bIncludePostTranslucent);
		Command.Execute(*RenderParameters.RHICmdList);
	}
	else
	{
	    new (RenderParameters.RHICmdList->AllocCommand<FRHIPostOpaqueCommand>()) FRHIPostOpaqueCommand(RenderParameters,this,uid,StereoPass,bMultiResEnabled,GFrameNumber,bIncludePostTranslucent);
	}
}

void FTrueSkyPlugin::UpdateTrueSkyLights(FRenderDelegateParameters& RenderParameters,int FrameNumber)
{
	static FVector ue_pos(0.0f,1.0f,2.0f),ts_pos,ue_pos2;
	ts_pos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_pos);
	ue_pos2=TrueSkyToUEPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ts_pos);
	TArray<UTrueSkyLightComponent*> &components=UTrueSkyLightComponent::GetTrueSkyLightComponents();
	int cube_id=0;
	FMatrix transf=UEToTrueSkyMatrix(false);
	TArray<FTextureRHIRef> unrefTextures;
	UTrueSkyLightComponent::AllSkylightsValid=true;
	if(StaticCopySkylight2|| StaticCopySkylight)
	for(int i=0;i<components.Num();i++)
	{
		UTrueSkyLightComponent* trueSkyLightComponent = components[i];
		FSkyLightSceneProxy *renderProxy=trueSkyLightComponent->GetSkyLightSceneProxy();
		if(renderProxy&&StaticCopySkylight)
		{
			trueSkyLightComponent->SetIntensity(1.0f);
			void *pContext= GetPlatformContext(RenderParameters, CurrentRHIType);
			const int nums=renderProxy->IrradianceEnvironmentMap.R.MaxSHBasis;
			int NumTotalFloats=renderProxy->IrradianceEnvironmentMap.R.NumSIMDVectors*renderProxy->IrradianceEnvironmentMap.R.NumComponentsPerSIMDVector;
			float shValues[nums*3];
			// We copy the skylight. The Skylight itself will be rendered with the specified SpecularMultiplier
			// because it will be copied directly to the renderProxy's texture.
			// returns 0 if successful:
			FTexture *t=renderProxy->ProcessedTexture;
			if(!t)
				continue;
			FLinearColor groundColour = trueSkyLightComponent->LowerHemisphereColor;
		
			void* tex=GetPlatformTexturePtr(t, CurrentRHIType);
			int32 f=trueSkyLightComponent->UpdateFrequency;
			float b=trueSkyLightComponent->Blend;
			float s=trueSkyLightComponent->SpecularMultiplier*actorCrossThreadProperties->Brightness;
			int32 order=renderProxy->IrradianceEnvironmentMap.R.MaxSHOrder;
			int result = 1;
			if(!trueSkyLightComponent->IsInitialized())
			{
				TriggerAction("ResetSkylights");
				trueSkyLightComponent->SetInitialized(true);
			}
			if (StaticCopySkylight2)
			{
				FTextureRHIRef *r=skylightTargetTextures.Find(cube_id);
				if(r&&r->IsValid())
				{
					if(r->GetReference()!=t->TextureRHI)
					{
						FTextureRHIRef u=*r;
						unrefTextures.Add(u);
						r->SafeRelease();
					}
				}
				FTextureRHIRef ref(t->TextureRHI);
				skylightTargetTextures.FindOrAdd(cube_id)=ref;
				if(actorCrossThreadProperties->CaptureCubemapRT!=nullptr&&actorCrossThreadProperties->CaptureCubemapTrueSkyLightComponent==trueSkyLightComponent)
				{
					int32 hires_cube_id=1835697618;
					auto r2=skylightTargetTextures.Find(hires_cube_id);
					if(r2&&r2->IsValid())
					{
						if(r2->GetReference()!=actorCrossThreadProperties->CaptureCubemapRT.GetReference())
							unrefTextures.Add(r2->GetReference());
					}
					skylightTargetTextures.FindOrAdd(hires_cube_id)=FTextureRHIRef(actorCrossThreadProperties->CaptureCubemapRT.GetReference());
					void* tex2=GetPlatformTexturePtr(actorCrossThreadProperties->CaptureCubemapRT.GetReference(), CurrentRHIType);
					if(tex2)
						StaticCopySkylight2(pContext
							, hires_cube_id
							, nullptr
							, order
							, tex2
							, &(transf.M[0][0])
							, 0					// For now, Update Frequency of zero shall be taken to mean: do a full render of the cube.
							, 0
							, 1.0f
							, 1.0f
							, (float*)&groundColour);
					f=0;	// force full init of skylight.
				}
				result = StaticCopySkylight2(pContext
					, cube_id
					, shValues
					, order
					, tex
					, &(transf.M[0][0])
					, f
					, b
					, s
					, 1.0f
					, (float*)&groundColour);
			}
			else
				result=StaticCopySkylight(pContext
									,cube_id
									,shValues
									,order
									,tex
									,&(transf.M[0][0])
									,f
									,b
									,s
									,1.0f);
			if(!result)
			{
				// we apply the diffuse multiplier in:
				trueSkyLightComponent->UpdateDiffuse(shValues);
#ifdef SIMUL_ENLIGHTEN_SUPPORT
				// we want to probe into the cubemap and get values to be stored in the 16x16x6 CPU-side cubemap.
				FLinearColor lc(0,0,0,0);
				// Returns 0 if successful.
				int res=StaticProbeSkylight(pContext
					,cube_id
					,EnlightenEnvironmentDataSize
					,faceIndex
					,x
					,y
					,1
					,1
					,(float*) &lc);
				if(res==0)
				{
					trueSkyLightComponent->UpdateEnlighten(faceIndex,x,y,1,1,&lc);
				}
#endif
			}
			else
			{
				UTrueSkyLightComponent::AllSkylightsValid=false;
			}
		}
		cube_id++;
	}
	for(auto r:unrefTextures)
	{
		r.SafeRelease();
	}
#ifdef SIMUL_ENLIGHTEN_SUPPORT
	x++;
	if(x>=EnlightenEnvironmentResolution)
	{
		x=0;
		y++;
		if(y>=EnlightenEnvironmentResolution)
		{
			y=0;
			faceIndex++;
			faceIndex%=6;
		}
	}

#endif
}
extern simul::QueryMap truesky_queries;

void FTrueSkyPlugin::RenderFrame(uint64_t uid,FRenderDelegateParameters& RenderParameters
														,EStereoscopicPass StereoPass
														,bool bMultiResEnabled
														,int FrameNumber)
{
#if TRUESKY_PLATFORM_SUPPORTED
	if(!StaticSetRenderTexture)
		return;
	errno=0;
	if(RenderParameters.Uid==0)
	{
		if(StaticShutDownInterface)
			StaticShutDownInterface();
		sequenceInUse=nullptr;
		return;
	}
	if(!RenderParameters.ViewportRect.Width()||!RenderParameters.ViewportRect.Height())
		return;
	UpdateFromActor();
	if(!actorCrossThreadProperties->Visible||!RenderingEnabled)
	{
		if(UTrueSkyLightComponent::AllSkylightsValid&&(actorCrossThreadProperties->CaptureCubemapRT==nullptr||actorCrossThreadProperties->CaptureCubemapTrueSkyLightComponent==nullptr))
			return;
	}
	if(actorCrossThreadProperties->activeSequence)
	{
		if (!actorCrossThreadProperties->activeSequence->IsValidLowLevel())
			return;
	}
	else
		return;
	Viewport viewports[3];
	viewports[0].x=RenderParameters.ViewportRect.Min.X;
	viewports[0].y=RenderParameters.ViewportRect.Min.Y;
	viewports[0].w=RenderParameters.ViewportRect.Width();
	viewports[0].h=RenderParameters.ViewportRect.Height();
	viewports[0].zmin = 0.0f;
	viewports[0].zmax = 1.0f;
	// VR Viewports:
	for(int i=1;i<3;i++)
	{
		viewports[i].x=(i==2?RenderParameters.ViewportRect.Width()/2:0);
		viewports[i].y=RenderParameters.ViewportRect.Min.Y;
		viewports[i].w=RenderParameters.ViewportRect.Width()/2;
		viewports[i].h=RenderParameters.ViewportRect.Height();
		viewports[i].zmin = 0.0f;
		viewports[i].zmax = 1.0f;
	}

	FMatrix viewMatrix		= RenderParameters.ViewMatrix;
	void* colourTarget		= GetColourTarget(RenderParameters,CurrentRHIType);
#if !SIMUL_BINARY_PLUGIN
	void* colourTargetTexture = GetPlatformTexturePtr(RenderParameters.RenderTargetTexture, CurrentRHIType);
#else
	void* colourTargetTexture=nullptr;
#endif
	void* depthTexture		= GetPlatformTexturePtr(RenderParameters.DepthTexture, CurrentRHIType);

#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		PostOpaquetStoreStateD3D12(&RenderParameters.RHICmdList->GetContext(), RenderParameters.RenderTargetTexture,RenderParameters.DepthTexture);
		if (actorCrossThreadProperties->CloudShadowRT)
		{
			FRHITexture2D* shadowRtRef = (FRHITexture2D*)actorCrossThreadProperties->CloudShadowRT.GetReference();
			RHIResourceTransitionD3D12(&RenderParameters.RHICmdList->GetContext(), (D3D12_RESOURCE_STATES)0, (D3D12_RESOURCE_STATES)0x4, shadowRtRef);
		}
	}
#endif
	
	void* rain_cubemap_dx11				=GetPlatformTexturePtr(actorCrossThreadProperties->RainCubemap, CurrentRHIType);
	void *moon_texture_dx11				=GetPlatformTexturePtr(actorCrossThreadProperties->MoonTexture, CurrentRHIType);
	void *cosmic_texture_dx11			=GetPlatformTexturePtr(actorCrossThreadProperties->CosmicBackgroundTexture, CurrentRHIType);
	void *loss_texture_dx11				=GetPlatformTexturePtr(actorCrossThreadProperties->LossRT, CurrentRHIType);
	void *insc_texture_dx11				=GetPlatformTexturePtr(actorCrossThreadProperties->InscatterRT, CurrentRHIType);
	void *cloud_vis_texture				=GetPlatformTexturePtr(actorCrossThreadProperties->CloudVisibilityRT, CurrentRHIType);

	StaticSetRenderTexture("Cubemap", rain_cubemap_dx11);
	StaticSetRenderTexture("Moon", moon_texture_dx11);
	StaticSetRenderTexture("Background", cosmic_texture_dx11);
	StaticSetRenderTexture("Loss2D"		,loss_texture_dx11);
	StaticSetRenderTexture("Inscatter2D",insc_texture_dx11);
	StaticSetRenderTexture("CloudVisibilityRT", cloud_vis_texture);

	if(simulVersion>=SIMUL_4_1)
	{
		void *cloud_shadow_texture			=GetPlatformTexturePtr(actorCrossThreadProperties->CloudShadowRT, CurrentRHIType);
		StaticSetRenderTexture("CloudShadowRT", cloud_shadow_texture);
	}

	if(actorCrossThreadProperties->SetTime)
		StaticSetRenderFloat("time",actorCrossThreadProperties->Time);
	StaticSetRenderFloat("sky:MaxSunRadiance"		, actorCrossThreadProperties->MaxSunRadiance);
	StaticSetRenderBool("sky:AdjustSunRadius"		, actorCrossThreadProperties->AdjustSunRadius);
	
	// Apply the sub-component volumes:
	criticalSection.Lock();
	for(auto i=cloudVolumes.begin();i!=cloudVolumes.end();i++)
	{
		Variant32 params[20];
		params[0].intVal=i->first;
		FMatrix m=i->second.transform.ToMatrixWithScale();
		RescaleMatrix(m,actorCrossThreadProperties->MetresPerUnit);
		for(int j=0;j<16;j++)
		{
			params[j+1].floatVal=((const float*)m.M)[j];
		}
		FVector ext=i->second.extents*actorCrossThreadProperties->MetresPerUnit;// We want this in metres.
		for(int j=0;j<3;j++)
		{
			params[17+j].floatVal=ext[j];
		}
		StaticSetRender("CloudVolume",20,params);
	}
	criticalSection.Unlock();
	FMatrix cubemapMatrix;
	cubemapMatrix.SetIdentity();
	AdaptCubemapMatrix(cubemapMatrix);
	StaticSetMatrix4x4("CubemapTransform", &(cubemapMatrix.M[0][0]));
	
	if(actorCrossThreadProperties->RainDepthRT)
	{
		StaticSetRenderTexture("RainDepthTexture",GetPlatformTexturePtr(actorCrossThreadProperties->RainDepthRT, CurrentRHIType));
		if(actorCrossThreadProperties->RainDepthRT)
		{
			FMatrix rainMatrix=actorCrossThreadProperties->RainDepthMatrix;		// The bottom row of this matrix is the worldspace position
	
			StaticSetMatrix4x4("RainDepthTransform", &(actorCrossThreadProperties->RainDepthMatrix.M[0][0]));
			StaticSetMatrix4x4("RainDepthProjection", &(actorCrossThreadProperties->RainProjMatrix.M[0][0]));
			StaticSetRenderFloat("RainDepthTextureScale",actorCrossThreadProperties->RainDepthTextureScale);
		}
	}
	float exposure=actorCrossThreadProperties->Brightness;
	static float g=1.0f;
	float gamma=g;

	FMatrix projMatrix = RenderParameters.ProjMatrix;
	AdaptProjectionMatrix(projMatrix,actorCrossThreadProperties->MetresPerUnit);
	void *device=GetPlatformDevice();
	void* pContext		= GetPlatformContext(RenderParameters, CurrentRHIType);
	AdaptViewMatrix(viewMatrix);
	PluginStyle style=UNREAL_STYLE;
	if(actorCrossThreadProperties->DepthBlending)
		style=style|DEPTH_BLENDING;
	
	if(actorCrossThreadProperties->ShareBuffersForVR)
	{
		if(StereoPass==eSSP_LEFT_EYE)
		{
			//whichever one comes first, use it to fill the buffers, then: 
			style=style|VR_STYLE;
			uid=29435;
		}
		else if(StereoPass==eSSP_RIGHT_EYE)
		{
			//share the generated buffers for the other one.
			style=style|VR_STYLE;
			uid=29435;
		}
		if(StereoPass==eSSP_LEFT_EYE||StereoPass==eSSP_RIGHT_EYE)
		{
			if(FrameNumber==LastStereoFrameNumber&&LastStereoFrameNumber!=-1)
				style=style|VR_STYLE_RIGHT_EYE;
		}
	}
	if(StereoPass==eSSP_RIGHT_EYE&&FrameNumber!=LastFrameNumber)
	{
		style=style|VR_STYLE_RIGHT_EYE;
	}
	// If not visible, don't write to the screen.
	if(!actorCrossThreadProperties->Visible)
	{
		style=style|DONT_COMPOSITE;
	}
    int view_id = StaticGetOrAddView((void*)uid);		// RVK: really need a unique view ident to pass here..
	int4 depthViewport;
	depthViewport.x=RenderParameters.ViewportRect.Min.X;
	depthViewport.y=RenderParameters.ViewportRect.Min.Y;
	depthViewport.z=RenderParameters.ViewportRect.Width();
	depthViewport.w=RenderParameters.ViewportRect.Height();
	// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
	TArray<FVector2D>  MultiResConstants;
#ifdef NV_MULTIRES
	if (bMultiResEnabled)
	{
		FSceneView *View = (FSceneView*)(RenderParameters.Uid);
		SetMultiResConstants(MultiResConstants, View);
	}
#endif
	// NVCHANGE_END: TrueSky + VR MultiRes Support
	

#if PLATFORM_PS4
		FGnmCommandListContext *CommandListContext=(FGnmCommandListContext*)(&RenderParameters.RHICmdList->GetContext());
		sce::Gnmx::LightweightGfxContext *lwgfxc=&(CommandListContext->GetContext());
#else
	IRHICommandContext *CommandListContext=(IRHICommandContext*)(&RenderParameters.RHICmdList->GetContext());
#endif
	RenderCloudShadow();

	{
		StaticTriggerAction("CalcRealTime"); 
	}
	StoreUEState(RenderParameters);
	if(actorCrossThreadProperties->CaptureCubemapRT!=nullptr&&actorCrossThreadProperties->CaptureCubemapTrueSkyLightComponent!=nullptr)
	{
		UpdateTrueSkyLights(RenderParameters,FrameNumber);
	}

	if (StaticRenderFrame2)
	{
		RenderFrameStruct frameStruct;
		frameStruct.colourTarget = colourTarget;
		frameStruct.device = device;
		frameStruct.pContext = pContext;
		frameStruct.view_id = view_id;
		frameStruct.viewMatrix4x4 = &(viewMatrix.M[0][0]);
		frameStruct.projMatrix4x4 = &(projMatrix.M[0][0]);
		frameStruct.depthTexture = depthTexture;
		frameStruct.colourTarget = colourTarget;
		frameStruct.colourTargetTexture = colourTargetTexture;
		frameStruct.isColourTargetTexture = true;
		frameStruct.depthViewport = depthViewport;
		frameStruct.targetViewport = viewports;
		frameStruct.style = style;
		frameStruct.exposure = exposure;
		frameStruct.gamma = gamma;
		frameStruct.framenumber = FrameNumber;
		frameStruct.nvMultiResConstants = (bMultiResEnabled ? (const float*)(MultiResConstants.GetData()) : nullptr);

		StaticRenderFrame2(&frameStruct);
	}
	else
	{
		StaticRenderFrame(device,pContext,view_id, &(viewMatrix.M[0][0]), &(projMatrix.M[0][0])
			,depthTexture,colourTarget
			,depthViewport
			,viewports
			,style
			,exposure
			,gamma
			,FrameNumber
			// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
			, (bMultiResEnabled ? (const float*)(MultiResConstants.GetData()) : nullptr)
			// NVCHANGE_END: TrueSky + VR MultiRes Support
			);
	}
	if(StaticProcessQueries)
	{
		for(auto &q:truesky_queries)
		{
			StaticProcessQueries(1,&q.Value);
		}
	}
	RenderCloudShadow();
	if(FrameNumber!=LastFrameNumber)
	{
		// TODO: This should only be done once per frame.
		UpdateTrueSkyLights(RenderParameters,FrameNumber);
		LastFrameNumber=FrameNumber;
		actorCrossThreadProperties->CaptureCubemapTrueSkyLightComponent.Reset();
		actorCrossThreadProperties->CaptureCubemapRT.SafeRelease();
	}
	
// Fill in colours requested by the editor plugin.
	if(colourTableRequests.size())
	{
		for(auto i:colourTableRequests)
		{
			unsigned uidc=i.first;
			ColourTableRequest *req=i.second;
			if(req&&req->valid)
				continue;
			delete [] req->data;
			req->data=new float[4*req->x*req->y*req->z];
			if(StaticFillColourTable(uidc,req->x,req->y,req->z,req->data))
			{
				UE_LOG(TrueSky,Display, TEXT("Colour table filled!"));
				req->valid=true;
				break;
			}
			else
			{
				UE_LOG(TrueSky,Display, TEXT("Colour table not filled!"));
				req->valid=false;
				continue;
			}
		}

		// tell the editor that the work is done.
		if(TrueSkyEditorCallback)
			TrueSkyEditorCallback();
	}
	if(exportNext)
	{
		StaticExportCloudLayerToGeometry(exportFilenameUtf8,0);
		exportNext=false;
	}
	// TODO: What we should really do now is to unset all resources on RenderParameters.RHICmdList, just in case
	// UE THINKS that it still has values set, that are now no longer really set in the (say) DX11 API.
//	RenderParameters.RHICmdList->FlushComputeShaderCache();
	if (StereoPass == eSSP_LEFT_EYE || StereoPass == eSSP_RIGHT_EYE)
		LastStereoFrameNumber = FrameNumber;
	LastFrameNumber = FrameNumber;

	RestoreUEState(RenderParameters,CurrentRHIType);


#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		PostOpaquetRestoreStateD3D12(&RenderParameters.RHICmdList->GetContext(), RenderParameters.RenderTargetTexture, RenderParameters.DepthTexture);
		if (actorCrossThreadProperties->CloudShadowRT)
		{
			FRHITexture2D* shadowRtRef = (FRHITexture2D*)actorCrossThreadProperties->CloudShadowRT.GetReference();
			if(shadowRtRef)
				RHIResourceTransitionD3D12(&RenderParameters.RHICmdList->GetContext(), (D3D12_RESOURCE_STATES)0x4, (D3D12_RESOURCE_STATES)(((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800), shadowRtRef);
		}
	}
#endif

#endif
}

void FTrueSkyPlugin::OnDebugTrueSky(class UCanvas* Canvas, APlayerController*)
{
	const FColor OldDrawColor = Canvas->DrawColor;
	const FFontRenderInfo FontRenderInfo = Canvas->CreateFontRenderInfo(false, true);

	Canvas->SetDrawColor(FColor::White);

	UFont* RenderFont = GEngine->GetSmallFont();
	Canvas->DrawText(RenderFont, FString("trueSKY Debug Display"), 0.3f, 0.3f, 1.f, 1.f, FontRenderInfo);

	Canvas->SetDrawColor(OldDrawColor);
}

void FTrueSkyPlugin::ShutdownModule()
{
	delete actorCrossThreadProperties;
	actorCrossThreadProperties=nullptr;
	if(StaticShutDownInterface)
		StaticShutDownInterface();
#if WITH_EDITOR
	// unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "trueSKY");
	}
#endif //WITH_EDITOR
	if(StaticSetGraphicsDevice)
#if PLATFORM_PS4
		StaticSetGraphicsDevice(nullptr, GraphicsDevicePS4, GraphicsEventShutdown);
#else
		StaticSetGraphicsDevice(nullptr, CurrentRHIType, GraphicsEventShutdown);
#endif
	delete PathEnv;
	CloudTexture3D=nullptr;
	PathEnv = nullptr;
	RendererInitialized=false;
	sequenceInUse=nullptr;
	if(StaticSetDebugOutputCallback)
		StaticSetDebugOutputCallback(nullptr);
	for(auto t:skylightTargetTextures)
	{
		t.Value.SafeRelease();
	}
	skylightTargetTextures.Empty();
	// Delete ctp last because it has refs that the other shutdowns might expect to be valid.
	delete actorCrossThreadProperties;
	actorCrossThreadProperties=nullptr;
}
#ifdef _MSC_VER
#if !PLATFORM_UWP
/** Returns environment variable value */
static wchar_t* GetEnvVariable( const wchar_t* const VariableName, int iEnvSize = 1024)
{
	wchar_t* Env = new wchar_t[iEnvSize];
	check( Env );
	memset(Env, 0, iEnvSize * sizeof(wchar_t));
	if ( (int)GetEnvironmentVariableW(VariableName, Env, iEnvSize) > iEnvSize )
	{
		delete [] Env;
		Env = nullptr;
	}
	else if ( wcslen(Env) == 0 )
	{
		return nullptr;
	}
	return Env;
}

#endif
#endif
/** Takes Base path, concatenates it with Relative path */
static const TCHAR* ConstructPath(const TCHAR* const BasePath, const TCHAR* const RelativePath)
{
	if ( BasePath )
	{
		const int iPathLen = 1024;
		TCHAR* const NewPath = new TCHAR[iPathLen];
		check( NewPath );
		wcscpy_s( NewPath, iPathLen, BasePath );
		if ( RelativePath )
		{
			wcscat_s( NewPath, iPathLen, RelativePath );
		}
		return NewPath;
	}
	return nullptr;
}
/** Takes Base path, concatenates it with Relative path and returns it as 8-bit char string */
static std::string ConstructPathUTF8(const TCHAR* const BasePath, const TCHAR* const RelativePath)
{
	if ( BasePath )
	{
		const int iPathLen = 1024;

		TCHAR* const NewPath = new TCHAR[iPathLen];
		check( NewPath );
		wcscpy_s( NewPath, iPathLen, BasePath );
		if ( RelativePath )
		{
			wcscat_s( NewPath, iPathLen, RelativePath );
		}

		char* const utf8NewPath = new char[iPathLen];
		check ( utf8NewPath );
		memset(utf8NewPath, 0, iPathLen);
#if PLATFORM_PS4 || PLATFORM_SWITCH
		size_t res=wcstombs(utf8NewPath, NewPath, iPathLen);
#else
		WideCharToMultiByte( CP_UTF8, 0, NewPath, iPathLen, utf8NewPath, iPathLen, nullptr, nullptr );
#endif

		delete [] NewPath;
		std::string ret=utf8NewPath;
		delete [] utf8NewPath;
		return ret;
	}
	return "";
}

bool CheckDllFunction(void *fn,FString &str,const char *fnName)
{
	bool res=(fn!=0);
	if(!res)
	{
		if(!str.IsEmpty())
			str+=", ";
		str+=fnName;
	}
	return res;
}
#if PLATFORM_PS4 || PLATFORM_SWITCH
#define MISSING_FUNCTION(fnName) (!CheckDllFunction((void*)fnName,failed_functions, #fnName))
#else
#define MISSING_FUNCTION(fnName) (!CheckDllFunction(fnName,failed_functions, #fnName))
#endif
// Reimplement because it's not there in the Xbox One version
//FWindowsPlatformProcess::GetDllHandle
moduleHandle GetDllHandle( const TCHAR* Filename )
{
	check(Filename);
#if PLATFORM_PS4
//moduleHandle ImageFileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*ImageFullPath);
	moduleHandle m= sceKernelLoadStartModule(TCHAR_TO_ANSI(Filename), 0, 0, 0, nullptr, nullptr);
	switch(m)
	{
		case SCE_KERNEL_ERROR_EINVAL:
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x80020016 flags or pOpt is invalid "));
			return 0;
		case SCE_KERNEL_ERROR_ENOENT:			 
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x80020002 File specified in moduleFileName does not exist "));
			return 0;
		case SCE_KERNEL_ERROR_ENOEXEC:			  
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error:  0x80020008 Cannot load because of abnormal file format "));
			return 0;
		case SCE_KERNEL_ERROR_ENOMEM:			  
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x8002000c Cannot load because it is not possible to allocate memory "));
			return 0;
		case SCE_KERNEL_ERROR_EFAULT:			  
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x8002000e moduleFileName points to invalid memory "));
			return 0;
		case SCE_KERNEL_ERROR_EAGAIN:			 
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x80020023 Cannot load because of insufficient resources "));
			return 0;
		default:
			break;
	};
	return m;
#else
#if PLATFORM_WINDOWS || PLATFORM_UWP
	moduleHandle h=FPlatformProcess::GetDllHandle(Filename);
	if (!h)
	{
		DWORD err = GetLastError();
		const int size = 1024;
		TCHAR *buffer=nullptr;
		if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER,
			nullptr
			,err
			,0
			,buffer
			,size
			,0))
		{
			return nullptr;
		}
		TCHAR tbuffer[1000];
		memcpy(tbuffer, buffer, 100);
		UE_LOG(TrueSky, Warning,tbuffer);

	}
	return h;
#elif PLATFORM_SWITCH
	UE_LOG_ONCE(TrueSky, Error, TEXT("GetDllHandle error: IMPLEMENT ME "));
	return nullptr;
#else
	return ::LoadLibraryW(Filename);
#endif
#endif
}

// And likewise for GetDllExport:
void* GetDllExport( moduleHandle DllHandle, const TCHAR* ProcName )
{
	check(DllHandle);
	check(ProcName);
#if PLATFORM_PS4
	void *addr=nullptr;
	int result=sceKernelDlsym( DllHandle,
								TCHAR_TO_ANSI(ProcName),
								&addr);
	/*SCE_KERNEL_ERROR_ESRCH 0x80020003 handle is invalid, or symbol specified in symbol is not exported 
SCE_KERNEL_ERROR_EFAULT 0x8002000e symbol or addrp address is invalid 

	*/
	if(result==SCE_KERNEL_ERROR_ESRCH)
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllExport got error: SCE_KERNEL_ERROR_ESRCH, which means 'handle is invalid, or symbol specified in symbol is not exported'"));
	}
	else if(result==SCE_KERNEL_ERROR_EFAULT)
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllExport got error: SCE_KERNEL_ERROR_EFAULT"));
	}
	return addr;
#elif PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_UWP
	return (void*)::GetProcAddress( (HMODULE)DllHandle, TCHAR_TO_ANSI(ProcName) );
#else
	return nullptr;
#endif
}
#define GET_FUNCTION(fnName) {fnName= (F##fnName)GetDllExport(DllHandle, TEXT(#fnName) );}

static const TCHAR PlatformName[] =
#if PLATFORM_XBOXONE
	TEXT("XboxOne");
#elif PLATFORM_PS4
	TEXT("PS4");
#elif PLATFORM_SWITCH
	TEXT("Switch");
#elif PLATFORM_UWP
	TEXT("UWP64");
#else
	TEXT("Win64");
#endif

bool FTrueSkyPlugin::InitRenderingInterface(  )
{
	InitPaths();
	static bool failed_once = false;
	FString EnginePath=FPaths::EngineDir();
#if PLATFORM_XBOXONE
	FString pluginFilename=TEXT("TrueSkyPluginRender_MD");
	FString debugExtension=TEXT("d");
	FString dllExtension=TEXT(".dll");
#endif
#if PLATFORM_PS4
	FString pluginFilename=TEXT("trueskypluginrender");
	FString debugExtension=TEXT("-debug");
	FString dllExtension=TEXT(".prx");
#endif
#if PLATFORM_WINDOWS || PLATFORM_UWP
	FString pluginFilename=TEXT("TrueSkyPluginRender_MT");
	FString debugExtension=TEXT("d");
	FString dllExtension=TEXT(".dll");
#endif
#if PLATFORM_SWITCH
	FString pluginFilename = TEXT("trueskypluginrender");
	FString debugExtension = TEXT("d");
	FString dllExtension = TEXT(".dll");
#endif
#ifndef NDEBUG // UE_BUILD_DEBUG //doesn't work... why?
	pluginFilename+=debugExtension;
#endif
	pluginFilename+=dllExtension;
	FString DllPath(FPaths::Combine(*EnginePath,TEXT("/binaries/thirdparty/simul/"),(const TCHAR *)PlatformName));
	FString DllFilename(FPaths::Combine(*DllPath,*pluginFilename));
#if PLATFORM_PS4
	if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
	{
		//   ../../../engine/plugins/trueskyplugin/alternatebinariesdir/ps4/trueskypluginrender.prx
		// For SOME reason, UE moves all prx's to the root/prx folder...??
		DllPath=TEXT("prx/");
		DllFilename=FPaths::Combine(*DllPath,*pluginFilename);
		if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
		{
			DllPath=FPaths::Combine(*EnginePath,TEXT("../prx/"));
			DllFilename=FPaths::Combine(*DllPath,*pluginFilename);
		}
	}
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
	{
		DllPath = "/app0/prx/";
		DllFilename = FPaths::Combine(*DllPath, *pluginFilename);
	}
#endif
	if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
	{
		if (!failed_once)
			UE_LOG(TrueSky, Warning, TEXT("%s not found."), *DllFilename);

		failed_once = true;
		return false;
	}
#if PLATFORM_PS4
	IFileHandle *fh=FPlatformFileManager::Get().GetPlatformFile().OpenRead(*DllFilename);
	delete fh;
	DllFilename	=FPlatformFileManager::Get().GetPlatformFile().ConvertToAbsolutePathForExternalAppForRead(*DllFilename);
#endif
	FPlatformProcess::PushDllDirectory(*DllPath);
	moduleHandle const DllHandle = GetDllHandle((const TCHAR*)DllFilename.GetCharArray().GetData() );
	FPlatformProcess::PopDllDirectory(*DllPath);
	if(DllHandle==0)
	{
		if(!failed_once)
			UE_LOG(TrueSky, Warning, TEXT("Failed to load %s"), (const TCHAR*)DllFilename.GetCharArray().GetData());
		failed_once = true;
		return false;
	}
	if (DllHandle != 0)
	{
		StaticInitInterface				=(FStaticInitInterface)GetDllExport(DllHandle, TEXT("StaticInitInterface") );
		StaticSetMemoryInterface		=(FStaticSetMemoryInterface)GetDllExport(DllHandle, TEXT("StaticSetMemoryInterface") );
		StaticShutDownInterface			=(FStaticShutDownInterface)GetDllExport(DllHandle, TEXT("StaticShutDownInterface") );
		StaticSetDebugOutputCallback	=(FStaticSetDebugOutputCallback)GetDllExport(DllHandle,TEXT("StaticSetDebugOutputCallback"));
		StaticSetGraphicsDevice			=(FStaticSetGraphicsDevice)GetDllExport(DllHandle, TEXT("StaticSetGraphicsDevice") );
		StaticSetGraphicsDeviceAndContext			=(FStaticSetGraphicsDeviceAndContext)GetDllExport(DllHandle, TEXT("StaticSetGraphicsDeviceAndContext") );
		StaticPushPath					=(FStaticPushPath)GetDllExport(DllHandle, TEXT("StaticPushPath") );

		GET_FUNCTION(StaticSetFileLoader);
		GET_FUNCTION(StaticGetLightningBolts);
		GET_FUNCTION(StaticSpawnLightning);
		GET_FUNCTION(StaticFillColourTable);
		GET_FUNCTION(GetSimulVersion);
		
		GET_FUNCTION(StaticGetOrAddView);
		GET_FUNCTION(StaticRenderFrame);
		GET_FUNCTION(StaticRenderFrame2);
		GET_FUNCTION(StaticCopySkylight);
		GET_FUNCTION(StaticCopySkylight2);
		GET_FUNCTION(StaticProbeSkylight);
		GET_FUNCTION(StaticRenderOverlays);
			
		GET_FUNCTION(StaticOnDeviceChanged);
		GET_FUNCTION(StaticTick);
		GET_FUNCTION(StaticGetEnvironment);
		GET_FUNCTION(StaticSetSequence);
		GET_FUNCTION(StaticGetRenderInterfaceInstance);

		GET_FUNCTION(StaticSetPointLight);
		GET_FUNCTION(StaticCloudPointQuery);
		GET_FUNCTION(StaticLightingQuery);
		GET_FUNCTION(StaticCloudLineQuery);
		GET_FUNCTION(StaticSetRenderTexture);
		GET_FUNCTION(StaticSetMatrix4x4);
		GET_FUNCTION(StaticSetRender);
		GET_FUNCTION(StaticSetRenderBool);
		GET_FUNCTION(StaticGetRenderBool);
		GET_FUNCTION(StaticSetRenderFloat);
		GET_FUNCTION(StaticGetRenderFloat);
		GET_FUNCTION(StaticSetRenderInt);
		GET_FUNCTION(StaticGetRenderInt);
					
		GET_FUNCTION(StaticCreateBoundedWaterObject);
		GET_FUNCTION(StaticRemoveBoundedWaterObject);

		GET_FUNCTION(StaticSetWaterFloat);
		GET_FUNCTION(StaticSetWaterInt);
		GET_FUNCTION(StaticSetWaterBool);
		GET_FUNCTION(StaticSetWaterVector);
		GET_FUNCTION(StaticGetWaterFloat);
		GET_FUNCTION(StaticGetWaterInt);
		GET_FUNCTION(StaticGetWaterBool);
		GET_FUNCTION(StaticGetWaterVector);
					
		GET_FUNCTION(StaticGetRenderString);		
		GET_FUNCTION(StaticSetRenderString);		

		GET_FUNCTION(StaticExportCloudLayerToGeometry);

		GET_FUNCTION(StaticTriggerAction);

		StaticSetKeyframeFloat			=(FStaticSetKeyframeFloat)GetDllExport(DllHandle, TEXT("StaticRenderKeyframeSetFloat"));
		StaticGetKeyframeFloat			=(FStaticGetKeyframeFloat)GetDllExport(DllHandle, TEXT("StaticRenderKeyframeGetFloat"));
		StaticSetKeyframeInt			=(FStaticSetKeyframeInt)GetDllExport(DllHandle,	TEXT("StaticRenderKeyframeSetInt"));
		StaticGetKeyframeInt			=(FStaticGetKeyframeInt)GetDllExport(DllHandle,	TEXT("StaticRenderKeyframeGetInt"));

		GET_FUNCTION(StaticGetRenderFloatAtPosition);
		GET_FUNCTION(StaticGetFloatAtPosition);
		GET_FUNCTION(StaticGet);
		GET_FUNCTION(StaticSet);
		GET_FUNCTION(StaticGetEnum);
		GET_FUNCTION(StaticProcessQueries);
		FString failed_functions;
		int num_fails=MISSING_FUNCTION(StaticInitInterface)
#if PLATFORM_PS4
			+MISSING_FUNCTION(StaticSetMemoryInterface)
#endif
			+MISSING_FUNCTION(StaticShutDownInterface)
			+MISSING_FUNCTION(StaticSetDebugOutputCallback)
			/*
			+MISSING_FUNCTION(StaticSetGraphicsDevice)
			+MISSING_FUNCTION(StaticSetGraphicsDeviceAndContext)
			*/
			+MISSING_FUNCTION(StaticPushPath)
			+MISSING_FUNCTION(StaticSetFileLoader)
			+MISSING_FUNCTION(StaticRenderFrame)
			+MISSING_FUNCTION(StaticGetLightningBolts)
			+MISSING_FUNCTION(StaticSpawnLightning)
			+MISSING_FUNCTION(StaticFillColourTable)
			+MISSING_FUNCTION(StaticRenderOverlays)
			+MISSING_FUNCTION(StaticGetOrAddView)
			+MISSING_FUNCTION(StaticGetEnvironment)
			+MISSING_FUNCTION(StaticSetSequence)
			+MISSING_FUNCTION(StaticGetRenderInterfaceInstance)
			+MISSING_FUNCTION(StaticSetPointLight)
			+MISSING_FUNCTION(StaticCloudPointQuery)
			+MISSING_FUNCTION(StaticCloudLineQuery)
			+MISSING_FUNCTION(StaticSetRenderTexture)
			+MISSING_FUNCTION(StaticSetRenderBool)
			+MISSING_FUNCTION(StaticGetRenderBool)
			+MISSING_FUNCTION(StaticTriggerAction)
			+MISSING_FUNCTION(StaticSetRenderFloat)
			+MISSING_FUNCTION(StaticGetRenderFloat)
			+MISSING_FUNCTION(StaticSetRenderString)
			+MISSING_FUNCTION(StaticGetRenderString)
			+MISSING_FUNCTION(StaticExportCloudLayerToGeometry)
			+MISSING_FUNCTION(StaticSetRenderInt)
			+MISSING_FUNCTION(StaticGetRenderInt)
			+MISSING_FUNCTION(StaticSetKeyframeFloat)
			+MISSING_FUNCTION(StaticGetKeyframeFloat)
			+MISSING_FUNCTION(StaticSetKeyframeInt)
			+MISSING_FUNCTION(StaticGetKeyframeInt)
			+MISSING_FUNCTION(StaticSetMatrix4x4)
			+MISSING_FUNCTION(StaticGetRenderFloatAtPosition)
			//+MISSING_FUNCTION(StaticGetFloatAtPosition)
			//+MISSING_FUNCTION(StaticGetFloatAtPosition)
			+(StaticSetGraphicsDevice==nullptr&&StaticSetGraphicsDeviceAndContext==nullptr);

		if(num_fails>0)
		{
			static bool reported=false;
			if(!reported)
			{
				UE_LOG(TrueSky, Error
					,TEXT("Can't initialize the trueSKY rendering plugin dll because %d functions were not found - please update TrueSkyPluginRender_MT.dll.\nThe missing functions are %s.")
					,num_fails
					,*failed_functions
					);
				reported=true;
			}
			//missing dll functions... cancel initialization
			SetRenderingEnabled(false);
			return false;
		}
		int water_fails=
			MISSING_FUNCTION(StaticCreateBoundedWaterObject)
			+MISSING_FUNCTION(StaticRemoveBoundedWaterObject)
			+MISSING_FUNCTION(StaticSetWaterFloat)
			+MISSING_FUNCTION(StaticSetWaterInt)
			+MISSING_FUNCTION(StaticSetWaterBool)
			+MISSING_FUNCTION(StaticSetWaterVector)
			+MISSING_FUNCTION(StaticGetWaterFloat)
			+MISSING_FUNCTION(StaticGetWaterInt)
			+MISSING_FUNCTION(StaticGetWaterBool)
			+MISSING_FUNCTION(StaticGetWaterVector);

		UE_LOG(TrueSky, Display, TEXT("Loaded trueSKY dynamic library %s."), *DllFilename);
		int major=4,minor=0,build=0;
		if(GetSimulVersion)
		{
			GetSimulVersion(&major,&minor,&build);
			simulVersion=ToSimulVersion(major,minor,build);
			UE_LOG(TrueSky, Display, TEXT("Simul version %d.%d build %d"), major,minor,build);
		}
		if(water_fails==0)
		{
			UE_LOG(TrueSky, Display, TEXT("trueSKY water is supported."));
			SetWaterRenderingEnabled(true);
		}
#if PLATFORM_PS4
		StaticSetMemoryInterface(simul::ue4::getMemoryInterface());
#endif
		StaticSetDebugOutputCallback(LogCallback);
		return true;
	}
	return false;
}

void FTrueSkyPlugin::PlatformSetGraphicsDevice()
{
	void* currentDevice = GetPlatformDevice();
#if PLATFORM_PS4 || PLATFORM_SWITCH
	StaticSetGraphicsDevice(currentDevice, CurrentRHIType, GraphicsEventInitialize);
#else
	if (CurrentRHIType == GraphicsDeviceD3D11 || CurrentRHIType == GraphicsDeviceD3D11_FastSemantics)
	{
		StaticSetGraphicsDevice(currentDevice, CurrentRHIType, GraphicsEventInitialize);
	}
#if ( !SIMUL_BINARY_PLUGIN)
	else if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		StaticSetGraphicsDeviceAndContext(currentDevice, GetContextD3D12(GDynamicRHI->RHIGetDefaultContext()), CurrentRHIType, GraphicsEventInitialize);
	}
#endif
#endif
}

bool FTrueSkyPlugin::EnsureRendererIsInitialized()
{
	// Cache the current rendering API from the dynamic RHI
	if (CurrentRHIType == GraphicsDeviceUnknown)
	{
		const TCHAR* rhiName = GDynamicRHI->GetName();
		UE_LOG(TrueSky, Display, TEXT("trueSKY will use the %s rendering API."), rhiName);

		std::string ansiRhiName(TCHAR_TO_ANSI(rhiName));
		if (ansiRhiName == "D3D11")
		{
#if USE_FAST_SEMANTICS_RENDER_CONTEXTS
			CurrentRHIType = GraphicsDeviceD3D11_FastSemantics;
#else
			CurrentRHIType = GraphicsDeviceD3D11;
#endif
		}
		else if (ansiRhiName == "D3D12")
		{
			CurrentRHIType = GraphicsDeviceD3D12;
		}
		else if (ansiRhiName == "GNM")
		{
			CurrentRHIType = GraphicsDevicePS4;
		}
	}

	ERRNO_CLEAR
	if(!RendererInitialized)
	{
		if(InitRenderingInterface())
			RendererInitialized=true;
		if(!RendererInitialized)
			return false;
		void *device=GetPlatformDevice();
		
		if( device != nullptr )
		{
			FString EnginePath=FPaths::EngineDir();
			StaticSetFileLoader(&ue4SimulFileLoader);
			FString shaderbin(FPaths::Combine(*EnginePath, TEXT("plugins/trueskyplugin/shaderbin/"),PlatformName));
			FString texpath(FPaths::EngineDir()+L"/plugins/trueskyplugin/resources/media/textures");
			StaticPushPath("TexturePath", FStringToUtf8(texpath).c_str());
			FString texpath2(FPaths::Combine(*EnginePath,L"/plugins/trueskyplugin/resources/media/textures"));
			StaticPushPath("TexturePath", FStringToUtf8(texpath2).c_str());
#if PLATFORM_WINDOWS || PLATFORM_UWP
			if(haveEditor)
			{
				StaticPushPath("ShaderPath",FStringToUtf8(trueSkyPluginPath+"\\Resources\\Platform\\DirectX11\\HLSL").c_str());
				StaticPushPath("TexturePath",FStringToUtf8(trueSkyPluginPath+"\\Resources\\Media\\Textures").c_str());
			}
			else
			{
				static FString gamePath="../../";
				StaticPushPath("ShaderPath",FStringToUtf8(gamePath+L"\\content\\trueskyplugin\\platform\\directx11\\hlsl").c_str());
				StaticPushPath("TexturePath",FStringToUtf8(gamePath+L"\\content\\trueskyplugin\\resources\\media\\textures").c_str());
			}
#endif
			if (CurrentRHIType == GraphicsDeviceD3D12)
			{
				FString shaderBinFull = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*shaderbin);
				StaticPushPath("ShaderBinaryPath", FStringToUtf8(shaderBinFull).c_str());
			}
			else
			{
				StaticPushPath("ShaderBinaryPath",FStringToUtf8(shaderbin).c_str());
			}
#ifdef _MSC_VER
#if !PLATFORM_UWP
#if !UE_BUILD_SHIPPING
			// IF there's a "SIMUL" env variable, we can build shaders direct from there:
			FString SimulPath = GetEnvVariable(L"SIMUL");
			if (SimulPath.Len() > 0)
			{
				simul::internal_logs=true;
			}
#endif
#endif
#endif
		}
		PlatformSetGraphicsDevice();
		if( device != nullptr )
		{
			RendererInitialized = true;
		}
		else
			RendererInitialized=false;
	}
	if(RendererInitialized)
	{
		StaticInitInterface();
		RenderingEnabled=actorCrossThreadProperties->Visible;
	}
	return RendererInitialized;
}

void FTrueSkyPlugin::SetPointLight(int id,FLinearColor c,FVector ue_pos,float min_radius,float max_radius)
{
	if(!StaticSetPointLight)
		return;
	criticalSection.Lock();
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	FVector ts_pos=UEToTrueSkyPosition(A->Transform,A->MetresPerUnit,ue_pos);
	StaticSetPointLight(id,(const float*)&ue_pos,min_radius,max_radius,(const float*)&c);
	criticalSection.Unlock();
}

ITrueSkyPlugin::VolumeQueryResult FTrueSkyPlugin::GetStateAtPosition(int32 queryId,FVector ue_pos)
{
	VolumeQueryResult res;

	if(!StaticCloudPointQuery)
	{
		memset(&res,0,sizeof(res));
		return res;
	}
	criticalSection.Lock();
	FVector ts_pos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_pos);
	StaticCloudPointQuery(queryId,(const float*)&ts_pos,&res);
	criticalSection.Unlock();
	return res;
}

ITrueSkyPlugin::LightingQueryResult FTrueSkyPlugin::GetLightingAtPosition(int32 queryId,FVector ue_pos)
{
	LightingQueryResult res;

	if(!StaticLightingQuery)
	{
		memset(&res,0,sizeof(res));
		return res;
	}
	criticalSection.Lock();
	FVector ts_pos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_pos);
	StaticLightingQuery(queryId,(const float*)&ts_pos,&res);
	// Adjust for Brightness, so that light and rendered sky are consistent.
	for(int i=0;i<3;i++)
	{
		res.sunlight[i]	*=actorCrossThreadProperties->Brightness;
		res.moonlight[i]*=actorCrossThreadProperties->Brightness;
		res.ambient[i]	*=actorCrossThreadProperties->Brightness;
	}
	criticalSection.Unlock();
	return res;
}

float FTrueSkyPlugin::GetCloudinessAtPosition(int32 queryId,FVector ue_pos)
{
	if(!StaticCloudPointQuery)
		return 0.f;
	criticalSection.Lock();
	VolumeQueryResult res;
	FVector ts_pos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_pos);
	StaticCloudPointQuery(queryId,(const float*)&ts_pos,&res);
	criticalSection.Unlock();
	return FMath::Clamp(res.density, 0.0f, 1.0f);
}

float FTrueSkyPlugin::CloudLineTest(int32 queryId,FVector ue_StartPos,FVector ue_EndPos)
{
	if(!StaticCloudLineQuery)
		return 0.f;
	criticalSection.Lock();
	LineQueryResult res;
	FVector StartPos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_StartPos);
	FVector EndPos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_EndPos);
	StaticCloudLineQuery(queryId,(const float*)&StartPos,(const float*)&EndPos,&res);
	criticalSection.Unlock();
	return FMath::Clamp(res.density, 0.0f, 1.0f);
}

void FTrueSkyPlugin::InitPaths()
{
#ifdef _MSC_VER

	static bool doOnce = false;
	if (!doOnce) {
		FModuleManager::Get().AddBinariesDirectory(FPaths::Combine((const TCHAR*)*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/Simul/Win64")).GetCharArray().GetData(), false);
		doOnce = true;
	}
#endif
}

void FTrueSkyPlugin::OnToggleRendering()
{
	if ( UTrueSkySequenceAsset* const ActiveSequence = GetActiveSequence() )
	{
		SetRenderingEnabled(!RenderingEnabled);
		if(RenderingEnabled)
		{
			SequenceChanged();
		}
	}
	else if(RenderingEnabled)
	{
		// no active sequence, so disable rendering
		SetRenderingEnabled(false);
	}
}

void FTrueSkyPlugin::OnUIChangedSequence()
{
	SequenceChanged();
	// Make update instant, instead of gradual, if it's a change the user made.
	TriggerAction("Reset");
}

void FTrueSkyPlugin::OnUIChangedTime(float t)
{
	ATrueSkySequenceActor::SetTime(t);
	UTrueSkySequenceAsset* const ActiveSequence = GetActiveSequence();
	if(ActiveSequence)
	{
		actorCrossThreadProperties->Time=t;
		actorCrossThreadProperties->SetTime=true;
		SetRenderFloat("time",actorCrossThreadProperties->Time);
	}
}
	
void FTrueSkyPlugin::ExportCloudLayer(const FString& fname,int index)
{
	exportNext=true;
	std::string name=FStringToUtf8(fname);
	strcpy_s(exportFilenameUtf8,100,name.c_str());
}

void FTrueSkyPlugin::SequenceChanged()
{
	if(!StaticSetSequence)
		return;
	UTrueSkySequenceAsset* const ActiveSequence = GetActiveSequence();
	if(ActiveSequence&&StaticSetSequence)
	{
		if(ActiveSequence->SequenceText.Num()>0)
		{
			std::string SequenceInputText;
			SequenceInputText = std::string((const char*)ActiveSequence->SequenceText.GetData());
			if(StaticSetSequence(SequenceInputText)==0)
				sequenceInUse=ActiveSequence;
		}
	}
}

IMPLEMENT_TOGGLE(ShowFades)
IMPLEMENT_TOGGLE(ShowCompositing)
IMPLEMENT_TOGGLE(ShowWaterTextures)
IMPLEMENT_TOGGLE(Show3DCloudTextures)
IMPLEMENT_TOGGLE(Show2DCloudTextures)

bool FTrueSkyPlugin::IsToggleRenderingEnabled()
{
	if(GetActiveSequence())
	{
		return true;
	}
	// No active sequence found!
	SetRenderingEnabled(false);
	return false;
}

bool FTrueSkyPlugin::IsToggleRenderingChecked()
{
	return RenderingEnabled;
}

void FTrueSkyPlugin::OnAddSequence()
{
	ULevel* const Level = GWorld->PersistentLevel;
	ATrueSkySequenceActor* SequenceActor = nullptr;
	// Check for existing sequence actor
	for(int i = 0; i < Level->Actors.Num() && SequenceActor == nullptr; i++)
	{
		SequenceActor = Cast<ATrueSkySequenceActor>( Level->Actors[i] );
	}
	//if ( SequenceActor == nullptr )
	{
		// Add sequence actor
		SequenceActor=GWorld->SpawnActor<ATrueSkySequenceActor>(ATrueSkySequenceActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
//	else
	{
		// Sequence actor already exists -- error message?
	}
}

void FTrueSkyPlugin::OnSequenceDestroyed()
{
}

bool FTrueSkyPlugin::IsAddSequenceEnabled()
{
	// Returns false if TrueSkySequenceActor already exists!
	ULevel* const Level = GWorld->PersistentLevel;
	for(int i=0;i<Level->Actors.Num();i++)
	{
		if ( Cast<ATrueSkySequenceActor>(Level->Actors[i]) )
			return false;
	}
	return true;
}

void FTrueSkyPlugin::UpdateFromActor()
{
	if(sequenceInUse!=GetActiveSequence())
		SequenceChanged();
	if (!sequenceInUse) //Make sure there is a valid sequence actor
	{
		//return;
		if(actorCrossThreadProperties->Destroyed)
		{
			// Destroyed.
			actorCrossThreadProperties->Visible=false;
		}
	}

	if(actorCrossThreadProperties->Visible!=RenderingEnabled)
	{
		OnToggleRendering();
	}
	if(RenderingEnabled&&RendererInitialized)
	{
		if(simulVersion>=SIMUL_4_1)
		{
			StaticSetRenderInt("WorleyTextureSize",(int)GetDefault<UTrueSkySettings>()->WorleyTextureSize);
			StaticSetRenderInt("InterpolationMode",(int)actorCrossThreadProperties->InterpolationMode);
			StaticSetRenderInt("cloudsteps",(int)GetDefault<UTrueSkySettings>()->DefaultCloudSteps);
			StaticSetRenderInt("MaxViewAgeFrames", (int)GetDefault<UTrueSkySettings>()->MaxFramesBetweenViewUpdates);
			StaticSetRenderFloat("MinimumStarPixelSize",actorCrossThreadProperties->MinimumStarPixelSize);
			StaticSetRenderFloat("InterpolationTimeSeconds",actorCrossThreadProperties->InterpolationTimeSeconds);
			StaticSetRenderFloat("nearestrainmetres",actorCrossThreadProperties->RainNearThreshold*actorCrossThreadProperties->MetresPerUnit);
			StaticSetRenderFloat("interpolationtimedays", actorCrossThreadProperties->InterpolationTimeHours / 24.0f );
			//SetRenderInt("InterpolationSubdivisions",actorCrossThreadProperties->InterpolationSubdivisions);
			StaticSetRenderInt("PrecipitationOptions",actorCrossThreadProperties->PrecipitationOptions);
		}

		if((simulVersion>=SIMUL_4_2) && WaterRenderingEnabled)
		{
			StaticSetRenderBool("RenderWater", actorCrossThreadProperties->RenderWater);

			if (actorCrossThreadProperties->RenderWater)
			{
				StaticSetRenderBool("EnableBoundlessOcean", actorCrossThreadProperties->EnableBoundlessOcean);

				if (actorCrossThreadProperties->EnableBoundlessOcean)
				{
					StaticSetRenderFloat("oceanbeaufortScale", actorCrossThreadProperties->OceanBeaufortScale);
					StaticSetRenderFloat("oceanwinddirection", actorCrossThreadProperties->OceanWindDirection);
					StaticSetRenderFloat("oceanwinddependancy", actorCrossThreadProperties->OceanWindDependancy);
					StaticSetWaterVector("boundlessscattering", -1, &actorCrossThreadProperties->OceanScattering[0]);
					StaticSetWaterVector("boundlessabsorption", -1, &actorCrossThreadProperties->OceanAbsorption[0]);
					if (actorCrossThreadProperties->AdvancedWaterOptions)
					{
						StaticSetRenderFloat("OceanWindSpeed", actorCrossThreadProperties->OceanWindSpeed);
						StaticSetRenderFloat("OceanWaveAmplitude", actorCrossThreadProperties->OceanWaveAmplitude);
						StaticSetRenderFloat("OceanChoppyScale", actorCrossThreadProperties->OceanChoppyScale);
						StaticSetRenderFloat("OceanFoamHeight", actorCrossThreadProperties->OceanFoamHeight);
						StaticSetRenderFloat("OceanFoamChurn", actorCrossThreadProperties->OceanFoamChurn);
					}
				}

				int ID = 0;
				for (auto propertyItr = waterActorsCrossThreadProperties.CreateIterator(); propertyItr; ++propertyItr)
				{
					if (propertyItr.Value() != nullptr)
					{
						WaterActorCrossThreadProperties* pProperty = propertyItr.Value();
						ID = propertyItr.Key();
						StaticSetWaterBool("render", ID, pProperty->Render);
						if (pProperty->Render)
						{
							FVector tempLocation = (pProperty->Location + actorCrossThreadProperties->OceanOffset) * actorCrossThreadProperties->MetresPerUnit;
							StaticSetWaterVector("location", ID, &tempLocation[0]);
							StaticSetWaterVector("dimension", ID, &pProperty->Dimension[0]);
							StaticSetWaterVector("scattering", ID, &pProperty->Scattering[0]);
							StaticSetWaterVector("absorption", ID, &pProperty->Absorption[0]);
							StaticSetWaterFloat("beaufortscale", ID, pProperty->BeaufortScale);
							StaticSetWaterFloat("winddirection", ID, pProperty->WindDirection);
							StaticSetWaterFloat("winddependancy", ID, pProperty->WindDependency);
							StaticSetWaterFloat("salinity", ID, pProperty->Salinity);
							StaticSetWaterFloat("temperature", ID, pProperty->Temperature);
							if (pProperty->AdvancedWaterOptions)
							{
								StaticSetWaterFloat("windspeed", ID, pProperty->WindSpeed);
								StaticSetWaterFloat("waveamplitude", ID, pProperty->WaveAmplitude);
								StaticSetWaterFloat("choppyscale", ID, pProperty->ChoppyScale);
							}
						}
					}
				}
			}
		}

		StaticSetRenderFloat("SimpleCloudShadowing",actorCrossThreadProperties->SimpleCloudShadowing);
		StaticSetRenderFloat("SimpleCloudShadowSharpness",actorCrossThreadProperties->SimpleCloudShadowSharpness);
		StaticSetRenderFloat("CloudThresholdDistanceKm", actorCrossThreadProperties->CloudThresholdDistanceKm);

		if(simulVersion>=SIMUL_4_1)
		{
			StaticSetRenderInt("MaximumCubemapResolution",actorCrossThreadProperties->MaximumResolution);
			SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_DepthSamplingPixelRange,actorCrossThreadProperties->DepthSamplingPixelRange);
			StaticSetRenderBool("GridRendering", actorCrossThreadProperties->GridRendering);
			StaticSetRenderInt("GodraysGrid.x", actorCrossThreadProperties->GodraysGrid[0]);
			StaticSetRenderInt("GodraysGrid.y", actorCrossThreadProperties->GodraysGrid[1]);
			StaticSetRenderInt("GodraysGrid.z", actorCrossThreadProperties->GodraysGrid[2]);
			if(!GlobalOverlayFlag)
				GlobalOverlayFlag=StaticGetRenderBool("AnyOverlays");
		}
		else
		{
			SetRenderInt("Downscale",std::min(8,std::max(1,1920/actorCrossThreadProperties->MaximumResolution)));
		}
		actorCrossThreadProperties->CriticalSection.Lock();
		for(const auto &t:actorCrossThreadProperties->SetVectors)
		{
			int64 i=t.Key;
			const vec3 &v=t.Value;
			StaticSet(i,(const Variant*)&v);
		}
		for(const auto &t:actorCrossThreadProperties->SetFloats)
		{
			StaticSet( (int64)t.Key,(const Variant*)&t.Value);
		}
		for(const auto &t:actorCrossThreadProperties->SetInts)
		{
			StaticSet( (int64)t.Key,(const Variant*)&t.Value);
		}
		actorCrossThreadProperties->SetVectors.Empty();
		actorCrossThreadProperties->SetFloats.Empty();
		actorCrossThreadProperties->SetInts.Empty();
		actorCrossThreadProperties->CriticalSection.Unlock();
		StaticSetRenderInt("Amortization",actorCrossThreadProperties->Amortization);
		StaticSetRenderInt("AtmosphericsAmortization", actorCrossThreadProperties->AtmosphericsAmortization);
		int num_l=StaticGetLightningBolts(actorCrossThreadProperties->lightningBolts,4);
		for(int i=0;i<num_l;i++)
		{
			LightningBolt *l=&actorCrossThreadProperties->lightningBolts[i];
			FVector u=TrueSkyToUEPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,FVector(l->pos[0],l->pos[1],l->pos[2]));
			FVector v=TrueSkyToUEPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,FVector(l->endpos[0],l->endpos[1],l->endpos[2]));
			l->pos[0]=u.X;
			l->pos[1]=u.Y;
			l->pos[2]=u.Z;
			l->endpos[0]=v.X;
			l->endpos[1]=v.Y;
			l->endpos[2]=v.Z;
		}
		if(actorCrossThreadProperties->Reset)
		{
			StaticTriggerAction("Reset");
			actorCrossThreadProperties->Reset=false;
		}
	}
}

UTrueSkySequenceAsset* FTrueSkyPlugin::GetActiveSequence()
{
	return actorCrossThreadProperties->activeSequence;
}

FMatrix FTrueSkyPlugin::UEToTrueSkyMatrix(bool apply_scale) const
{
	FMatrix TsToUe	=actorCrossThreadProperties->Transform.ToMatrixWithScale();
	FMatrix UeToTs	=TsToUe.InverseFast();
	for(int i=apply_scale?0:3;i<4;i++)
	{
		for(int j=0;j<3;j++)
		{
			UeToTs.M[i][j]			*=actorCrossThreadProperties->MetresPerUnit;
		}
	}

	for(int i=0;i<4;i++)
		std::swap(UeToTs.M[i][0],UeToTs.M[i][1]);
	return UeToTs;
}

FMatrix FTrueSkyPlugin::TrueSkyToUEMatrix(bool apply_scale) const
{
	FMatrix TsToUe	=actorCrossThreadProperties->Transform.ToMatrixWithScale();
	for(int i=0;i<4;i++)
		std::swap(TsToUe.M[i][0],TsToUe.M[i][1]);
	for(int i=apply_scale?0:3;i<4;i++)
	{
		for(int j=0;j<3;j++)
		{
			TsToUe.M[i][j]			/=actorCrossThreadProperties->MetresPerUnit;
		}
	}
	return TsToUe;
}
namespace simul
{
	namespace unreal
	{
		FVector UEToTrueSkyPosition(const FTransform &tr,float MetresPerUnit,FVector ue_pos) 
		{
			FMatrix TsToUe	=tr.ToMatrixWithScale();
			FVector ts_pos	=tr.InverseTransformPosition(ue_pos);
			ts_pos			*=MetresPerUnit;
			std::swap(ts_pos.Y,ts_pos.X);
			return ts_pos;
		}

		FVector TrueSkyToUEPosition(const FTransform &tr,float MetresPerUnit,FVector ts_pos) 
		{
			ts_pos			/=actorCrossThreadProperties->MetresPerUnit;
			std::swap(ts_pos.Y,ts_pos.X);
			FVector ue_pos	=tr.TransformPosition(ts_pos);
			return ue_pos;
		}

		FVector UEToTrueSkyDirection(const FTransform &tr,FVector ue_dir) 
		{
			FMatrix TsToUe	=tr.ToMatrixNoScale();
			FVector ts_dir	=tr.InverseTransformVectorNoScale(ue_dir);
			std::swap(ts_dir.Y,ts_dir.X);
			return ts_dir;
		}

		FVector TrueSkyToUEDirection(const FTransform &tr,FVector ts_dir) 
		{
			std::swap(ts_dir.Y,ts_dir.X);
			FVector ue_dir	=tr.TransformVectorNoScale(ts_dir);
			return ue_dir;
		}
	}
}

#undef LOCTEXT_NAMESPACE
#undef LLOCTEXT_NAMESPACE