#include "TrueSkyPluginD3D12.h"
#ifndef PLATFORM_UWP
#define PLATFORM_UWP 0
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) && !SIMUL_BINARY_PLUGIN
	/*
		We define these macros here to prevent collisions with method names
		between dx11 and dx12
	*/
	#define FindShaderResourceDXGIFormat	FindShaderResourceDXGIFormat_D3D12
	#define FindUnorderedAccessDXGIFormat	FindUnorderedAccessDXGIFormat_D3D12
	#define FindDepthStencilDXGIFormat		FindDepthStencilDXGIFormat_D3D12
	#define HasStencilBits					HasStencilBits_D3D12
	#define GetRenderTargetFormat			GetRenderTargetFormat_D3D12
	#define FD3DGPUProfiler					FD3DGPUProfiler_D3D12
	#define FVector4VertexDeclaration		FVector4VertexDeclaration_D3D12

	#define Initialize						Initialize_D3D12_Custom
	#define CheckResourceState				CheckResourceState_D3D12_Custom
	#define ConditionalInitalize			ConditionalInitalize_D3D12_Custom 
	#define	AreAllSubresourcesSame			AreAllSubresourcesSame_D3D12_Custom
	#define	CheckResourceStateInitalized	CheckResourceStateInitalized_D3D12_Custom
	#define	GetSubresourceState				GetSubresourceState_D3D12_Custom
	#define	SetResourceState				SetResourceState_D3D12_Custom
	#define	SetResourceState				SetResourceState_D3D12_Custom
	#define	SetSubresourceState				SetSubresourceState_D3D12_Custom
	
	#define	UpdateResidency					UpdateResidency_D3D12_Custom

	#define	GetResourceState				GetResourceState_D3D12_Custom
	#define	AddTransitionBarrier			AddTransitionBarrier_D3D12_Custom

	#include "D3D12RHIPrivate.h"
	#include "D3D12StateCachePrivate.h"
	#include "D3D12CommandList.h"
	

// Duplicated from D3D12Util.cpp -------------------
void CResourceState::Initialize(uint32 SubresourceCount)
{
	check(0 == m_SubresourceState.Num());

	// Allocate space for per-subresource tracking structures
	check(SubresourceCount > 0);
	m_SubresourceState.SetNumUninitialized(SubresourceCount);
	check(m_SubresourceState.Num() == SubresourceCount);

	// All subresources start out in an unknown state
	SetResourceState(D3D12_RESOURCE_STATE_TBD);
}

bool CResourceState::AreAllSubresourcesSame() const
{
	return m_AllSubresourcesSame && (m_ResourceState != D3D12_RESOURCE_STATE_TBD);
}

bool CResourceState::CheckResourceStateInitalized() const
{
	return m_SubresourceState.Num() > 0;
}

bool CResourceState::CheckResourceState(D3D12_RESOURCE_STATES State) const
{
	if (m_AllSubresourcesSame)
	{
		return State == m_ResourceState;
	}
	else
	{
		// All subresources must be individually checked
		const uint32 numSubresourceStates = m_SubresourceState.Num();
		for (uint32 i = 0; i < numSubresourceStates; i++)
		{
			if (m_SubresourceState[i] != State)
			{
				return false;
			}
		}

		return true;
	}
}

D3D12_RESOURCE_STATES CResourceState::GetSubresourceState(uint32 SubresourceIndex) const
{
	if (m_AllSubresourcesSame)
	{
		return m_ResourceState;
	}
	else
	{
		check(SubresourceIndex < static_cast<uint32>(m_SubresourceState.Num()));
		return m_SubresourceState[SubresourceIndex];
	}
}

void CResourceState::SetResourceState(D3D12_RESOURCE_STATES State)
{
	m_AllSubresourcesSame = 1;

	m_ResourceState = State;

	// State is now tracked per-resource, so m_SubresourceState should not be read.
#if UE_BUILD_DEBUG
	const uint32 numSubresourceStates = m_SubresourceState.Num();
	for (uint32 i = 0; i < numSubresourceStates; i++)
	{
		m_SubresourceState[i] = D3D12_RESOURCE_STATE_CORRUPT;
	}
#endif
}

void CResourceState::SetSubresourceState(uint32 SubresourceIndex, D3D12_RESOURCE_STATES State)
{
	// If setting all subresources, or the resource only has a single subresource, set the per-resource state
	if (SubresourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES ||
		m_SubresourceState.Num() == 1)
	{
		SetResourceState(State);
	}
	else
	{
		check(SubresourceIndex < static_cast<uint32>(m_SubresourceState.Num()));

		// If state was previously tracked on a per-resource level, then transition to per-subresource tracking
		if (m_AllSubresourcesSame)
		{
			const uint32 numSubresourceStates = m_SubresourceState.Num();
			for (uint32 i = 0; i < numSubresourceStates; i++)
			{
				m_SubresourceState[i] = m_ResourceState;
			}

			m_AllSubresourcesSame = 0;

			// State is now tracked per-subresource, so m_ResourceState should not be read.
#if UE_BUILD_DEBUG
			m_ResourceState = D3D12_RESOURCE_STATE_CORRUPT;
#endif
		}

		m_SubresourceState[SubresourceIndex] = State;
	}
}


// End duplicated from D3D12Util.cpp -------------------

// Start Duplicated from D3D12Resource.cpp -------------------
void FD3D12Heap::UpdateResidency(FD3D12CommandListHandle& CommandList)
{
#if ENABLE_RESIDENCY_MANAGEMENT
	if (D3DX12Residency::IsInitialized(ResidencyHandle))
	{
		D3DX12Residency::Insert(CommandList.GetResidencySet(), ResidencyHandle);
	}
#endif
}


void FD3D12Resource::UpdateResidency(FD3D12CommandListHandle& CommandList)
{
#if ENABLE_RESIDENCY_MANAGEMENT
	if (IsPlacedResource())
	{
		Heap->UpdateResidency(CommandList);
	}
	else if (D3DX12Residency::IsInitialized(ResidencyHandle))
	{
		check(Heap == nullptr);
		D3DX12Residency::Insert(CommandList.GetResidencySet(), ResidencyHandle);
	}
#endif
}
// End Duplicated from D3D12Resource.cpp -------------------

// Start Duplicated from D3D12CommandList.cpp -------------------
CResourceState& FD3D12CommandListHandle::FD3D12CommandListData::FCommandListResourceState::GetResourceState(FD3D12Resource* pResource)
{
	// Only certain resources should use this
	check(pResource->RequiresResourceStateTracking());

	CResourceState& ResourceState = ResourceStates.FindOrAdd(pResource);
	ConditionalInitalize(pResource, ResourceState);
	return ResourceState;
}

void FD3D12CommandListHandle::AddTransitionBarrier(FD3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource)
{
	check(CommandListData);
	CommandListData->ResourceBarrierBatcher.AddTransition(pResource->GetResource(), Before, After, Subresource);
	CommandListData->CurrentOwningContext->numBarriers++;

	pResource->UpdateResidency(*this);
}

void inline FD3D12CommandListHandle::FD3D12CommandListData::FCommandListResourceState::ConditionalInitalize(FD3D12Resource* pResource, CResourceState& ResourceState)
{
	// If there is no entry, all subresources should be in the resource's TBD state.
	// This means we need to have pending resource barrier(s).
	if (!ResourceState.CheckResourceStateInitalized())
	{
		ResourceState.Initialize(pResource->GetSubresourceCount());
		check(ResourceState.CheckResourceState(D3D12_RESOURCE_STATE_TBD));
	}

	check(ResourceState.CheckResourceStateInitalized());
}
// End Duplicated from D3D12CommandList.cpp -------------------

	void* simul::GetContextD3D12(IRHICommandContext* cmdContext)
	{
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)cmdContext;
		if (cmdContext12->CommandListHandle.IsClosed())
		{
			return nullptr;
		}
		return cmdContext12->CommandListHandle.CommandList();
	}
	
	TArray<CD3DX12_CPU_DESCRIPTOR_HANDLE> cachedHandles;
	int curHandleIdx = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE* simul::GetRenderTargetD3D12(FRHITexture2D* tex)
	{
		if (cachedHandles.Num() == 0)
		{
			cachedHandles.Init(CD3DX12_CPU_DESCRIPTOR_HANDLE(),10);
			curHandleIdx = 0;
		}
		curHandleIdx = (curHandleIdx + 1) % 10;
		auto texD12		= (FD3D12Texture2D*)tex;
		cachedHandles[curHandleIdx] = texD12->GetRenderTargetView(0, 0)->GetView();
		return &cachedHandles[curHandleIdx];
	}
	
	void simul::RestoreHeapsD3D12(IRHICommandContext* cmdContext)
	{
		// This can be replaced by using DirtyState() only?
		FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)cmdContext;
		auto desCache						= cmdContext12->StateCache.GetDescriptorCache();

		// It has to be in the same order as in: D3D12DescriptorCache.cpp FD3D12DescriptorCache::SetDescriptorHeaps()
		ID3D12DescriptorHeap* pHeaps[]		= { desCache->GetViewDescriptorHeap(),desCache->GetSamplerDescriptorHeap() };
		cmdContext12->CommandListHandle->SetDescriptorHeaps(_countof(pHeaps), pHeaps);
		
		// Mark as dirty rendering state
		// cmdContext12->StateCache.ForceRebuildGraphicsPSO();
		// cmdContext12->StateCache.ForceRebuildComputePSO();
		cmdContext12->StateCache.ForceSetGraphicsRootSignature();
		cmdContext12->StateCache.ForceSetComputeRootSignature();
		cmdContext12->StateCache.ForceSetVB();
		cmdContext12->StateCache.ForceSetIB();
		cmdContext12->StateCache.ForceSetRTs();
		cmdContext12->StateCache.ForceSetSOs();
		cmdContext12->StateCache.ForceSetViewports();
		cmdContext12->StateCache.ForceSetScissorRects();
		cmdContext12->StateCache.ForceSetPrimitiveTopology();
		cmdContext12->StateCache.ForceSetBlendFactor();
		cmdContext12->StateCache.ForceSetStencilRef();
	}

	bool simul::EnsureWriteStateD3D12(IRHICommandContext* cmdContext, FRHITexture2D* res)
	{
		FD3D12TextureBase* base = GetD3D12TextureFromRHITexture(res);
		if (base)
		{
			FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)cmdContext;
			auto writeState						= base->GetResource()->GetWritableState();
			FD3D12DynamicRHI::TransitionResourceWithTracking(cmdContext12->CommandListHandle, base->GetResource(), writeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
			return true;
	}
		return false;
	}

	bool simul::EnsureReadStateD3D12(IRHICommandContext* cmdContext, FRHITexture2D* res)
	{
		FD3D12TextureBase* base = GetD3D12TextureFromRHITexture(res);
		if (base)
		{
			FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)cmdContext;
			auto readState						= base->GetResource()->GetReadableState();
			FD3D12DynamicRHI::TransitionResourceWithTracking(cmdContext12->CommandListHandle, base->GetResource(), readState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
			return true;
		}
		return false;
	}

	void simul::PostOpaquetStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
		{
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;

		EnsureWriteStateD3D12(ctx, colour);
		cmdContext12->CommandListHandle.FlushResourceBarriers();

		cmdContext12->CommandListHandle->OMSetRenderTargets(1, (D3D12_CPU_DESCRIPTOR_HANDLE*)GetRenderTargetD3D12(colour), false, nullptr);
	}

	void simul::PostOpaquetRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
	{

	}

	void simul::PostTranslucentStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
	{
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;

		EnsureWriteStateD3D12(ctx, colour);
		EnsureReadStateD3D12(ctx, depth);
		cmdContext12->CommandListHandle.FlushResourceBarriers();

		cmdContext12->CommandListHandle->OMSetRenderTargets(1, (D3D12_CPU_DESCRIPTOR_HANDLE*)GetRenderTargetD3D12(colour), false, nullptr);
	}

	void simul::PostTranslucentRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
	{
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;
		cmdContext12->CommandListHandle.FlushResourceBarriers();
	}
	
	void simul::OverlaysStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
	{
		EnsureWriteStateD3D12(ctx, colour);
		EnsureReadStateD3D12(ctx, depth);
	}

	void simul::OverlaysRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
	{
	}

	void simul::RHIResourceTransitionD3D12(IRHICommandContext* ctx, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, FRHITexture2D* texture)
	{
		FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)ctx;
		D3D12_RESOURCE_BARRIER barrier		= CD3DX12_RESOURCE_BARRIER::Transition((ID3D12Resource*)texture->GetNativeResource(), before, after);
		cmdContext12->CommandListHandle->ResourceBarrier(1, &barrier);
	}
	
	#undef Initialize
	#undef CheckResourceState	
	#undef ConditionalInitalize
	#undef AreAllSubresourcesSame			
	#undef CheckResourceStateInitalized	
	#undef GetSubresourceState				
	#undef SetResourceState				
	#undef SetResourceState				
	#undef SetSubresourceState			

#ifdef UpdateResidency
	#undef UpdateResidency					
#endif
	#undef GetResourceState				
	#undef AddTransitionBarrier			

	#undef FindShaderResourceDXGIFormat
	#undef FindUnorderedAccessDXGIFormat
	#undef FindDepthStencilDXGIFormat
	#undef HasStencilBits
	#undef GetRenderTargetFormat
	#undef FD3DGPUProfiler
	#undef FVector4VertexDeclaration

#endif