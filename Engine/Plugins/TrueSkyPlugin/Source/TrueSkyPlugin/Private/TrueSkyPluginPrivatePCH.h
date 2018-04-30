// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once
#include "ITrueSkyPlugin.h"

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.

#include "RenderResource.h"

DECLARE_LOG_CATEGORY_EXTERN(TrueSky, Log, All);

#ifndef UE_LOG_SIMUL_INTERNAL
#if !UE_BUILD_SHIPPING
#define UE_LOG_SIMUL_INTERNAL(a,b,c,d) { if(simul::internal_logs) {UE_LOG(a,b,c,d);}}
#endif	// !UE_BUILD_SHIPPING
#endif	// UE_LOG_SIMUL_INTERNAL.

namespace simul
{
	extern bool internal_logs;
	enum PluginStyle
	{
		DEFAULT_STYLE			= 0,
		UNREAL_STYLE			= 1,
		UNITY_STYLE				= 2,
		UNITY_STYLE_DEFERRED	= 6,
		VISION_STYLE			= 8,
		CUBEMAP_STYLE			= 16,
		VR_STYLE				= 32,	// without right eye flag, this means left eye.
		VR_STYLE_RIGHT_EYE		= 64,
		POST_TRANSLUCENT		= 128,	// rain, snow etc.
		VR_STYLE_SIDE_BY_SIDE	= 256,
		DEPTH_BLENDING			= 512,
		DONT_COMPOSITE			= 1024
	};

	inline PluginStyle operator|(PluginStyle a, PluginStyle b)
	{
		return static_cast<PluginStyle>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline PluginStyle operator&(PluginStyle a, PluginStyle b)
	{
		return static_cast<PluginStyle>(static_cast<int>(a) & static_cast<int>(b));
	}

	inline PluginStyle operator~(PluginStyle a)
	{
		return static_cast<PluginStyle>(~static_cast<int>(a));
	}

	//! Graphics API list
	enum GraphicsDeviceType
	{
		GraphicsDeviceUnknown				= -1,
		GraphicsDeviceOpenGL				=  0,	// Desktop OpenGL
		GraphicsDeviceD3D9					=  1,	// Direct3D 9
		GraphicsDeviceD3D11					=  2,	// Direct3D 11
		GraphicsDeviceGCM					=  3,	// PlayStation 3
		GraphicsDeviceNull					=  4,	// "null" device (used in batch mode)
		GraphicsDeviceXenon					=  6,	// Xbox 360
		GraphicsDeviceOpenGLES20			=  8,	// OpenGL ES 2.0
		GraphicsDeviceOpenGLES30			= 11,	// OpenGL ES 3.0
		GraphicsDeviceGXM					= 12,	// PlayStation Vita
		GraphicsDevicePS4					= 13,	// PlayStation 4
		GraphicsDeviceXboxOne				= 14,	// Xbox One        
		GraphicsDeviceMetal					= 16,	// iOS Metal
		GraphicsDeviceD3D12					= 18,	// Direct3D 12
		GraphicsDeviceD3D11_FastSemantics	= 1002, // Direct3D 11 (fast semantics)
	};

	//! Event types
	enum GraphicsEventType
	{
		GraphicsEventInitialize = 0,
		GraphicsEventShutdown	= 1,
		GraphicsEventBeforeReset= 2,
		GraphicsEventAfterReset	= 3
	};

	struct Viewport
	{
		int x,y,w,h;
	};

	struct LightningBolt
	{
		int id;
		float pos[3];
		float endpos[3];
		float brightness;
		float colour[3];
		int age;
	};

	struct Query
	{
		Query():Enum(0),LastFrame(0),Float(0.0f),uid(0),Valid(false){}
		int64 Enum;
		FVector Pos;
		int32 LastFrame;
		union
		{
			float Float;
			int32 Int;
		};
		int32 uid;
		bool Valid;
	};
	typedef TMap<int64,Query> QueryMap;
}
