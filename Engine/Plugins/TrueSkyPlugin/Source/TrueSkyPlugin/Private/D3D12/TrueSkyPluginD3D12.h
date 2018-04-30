/*
	TrueSkyPluginD3D12.h
*/

#pragma once
#include "ModifyDefinitions.h"
#ifndef PLATFORM_UWP
#define PLATFORM_UWP 0
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN

struct CD3DX12_CPU_DESCRIPTOR_HANDLE;
class FRHITexture2D;
class IRHICommandContext;
enum D3D12_RESOURCE_STATES;

namespace simul
{
	//! Returns the graphical command list referenced in the provided command context
	void* GetContextD3D12(IRHICommandContext* cmdContext);
	
	//! Returns the render target handle of the provided texture
	CD3DX12_CPU_DESCRIPTOR_HANDLE* GetRenderTargetD3D12(FRHITexture2D* tex);
	
	//! Restores UE4 descriptor heaps
	void RestoreHeapsD3D12(IRHICommandContext* cmdContext);

	bool EnsureWriteStateD3D12(IRHICommandContext* cmdContext, FRHITexture2D* res);

	bool EnsureReadStateD3D12(IRHICommandContext* cmdContext, FRHITexture2D* res);

	void PostOpaquetStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	void PostOpaquetRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	void PostTranslucentStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depthr);

	void PostTranslucentRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	void OverlaysStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	void OverlaysRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	//! Unreal's barrier system is WIP (and the d3d12rhi barriers force you to use a resource with tracking enabled) this
	//! method basically performs a d3d12 barrier on the ID3D12GraphicsCommandList
	void RHIResourceTransitionD3D12(IRHICommandContext* ctx, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, FRHITexture2D* texture);
}

#endif