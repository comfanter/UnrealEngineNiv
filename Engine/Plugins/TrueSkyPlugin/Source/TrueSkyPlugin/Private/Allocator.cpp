#include "Allocator.h"
#include "TrueSkyPluginPrivatePCH.h"
#include <map>
#include <vector>
using namespace simul;
using namespace base;
using namespace unreal;

static FString memstring(int bytes)
{
	FString memtxt;
	if(bytes<1024)
		memtxt+=FString::Printf(TEXT("%d bytes"),bytes);
	else
	{
		int k=bytes/1024;
		if(k<1024)
		{
			memtxt+=FString::Printf(TEXT("%dk"),k);
		}
		else
		{
			int meg=k/1024;
			memtxt+=FString::Printf(TEXT("%dM"),meg);
		}
	}
	return memtxt;
}

#if PLATFORM_PS4
#include "GnmMemory.h"

typedef SceKernelModule moduleHandle;
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Onion trueSKY"),STAT_Onion_trueSKY,STATGROUP_PS4RHI,FPlatformMemory::MCR_GPUSystem,);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Garlic trueSKY"),STAT_Garlic_trueSKY,STATGROUP_PS4RHI,FPlatformMemory::MCR_GPU,);
DEFINE_STAT(STAT_Onion_trueSKY);
DEFINE_STAT(STAT_Garlic_trueSKY);
struct cmpByStringAddr {
	bool operator()(const std::string& a, const std::string& b) const {
		return (a.c_str()<b.c_str());
	}
};
class MemoryAllocator:public MemoryInterface
{
	int maxVideoAllocated;
	int totalVideoAllocated;
	int totalVideoFreed;
	std::map<void*,FMemBlock> memBlocks;
	std::map<std::string,int,cmpByStringAddr> memoryTracks;
	std::map<void *,std::string> allocationNames;
public:
	MemoryAllocator()
		:maxVideoAllocated(0)
		,totalVideoAllocated(0)
		,totalVideoFreed(0)
	{
	}
	//! Allocate \a nbytes bytes of memory, aligned to \a align and return a pointer to them.
	virtual void* AllocateTracked(size_t nbytes,size_t align,const char *fn)
	{
		if(align==0)
			align=1;
		FMemBlock blck=FMemBlock::Allocate(nbytes,align,EGnmMemType::GnmMem_CPU,GET_STATID(STAT_Onion_trueSKY));
		void *ptr=blck.GetPointer();
		memBlocks[ptr]=blck;
		return ptr;
	}
	//! De-allocate the memory at \param address (requires that this memory was allocated with Allocate()).
	virtual void Deallocate(void* ptr)
	{
		if(ptr)
		{
			FMemBlock::Free(memBlocks[ptr]);
			memBlocks.erase(memBlocks.find(ptr));
		}
	}
	//! Allocate \a nbytes bytes of memory, aligned to \a align and return a pointer to them.
	virtual void* AllocateVideoMemoryTracked(size_t nbytes,size_t align,const char *fn)
	{
		if(align==0)
			align		=1;
		FMemBlock blck	=FMemBlock::Allocate(nbytes,align,EGnmMemType::GnmMem_GPU,GET_STATID(STAT_Garlic_trueSKY));
		void *ptr		=blck.GetPointer();
		if(ptr)
		{
#ifdef DEBUG
			if(GetCurrentVideoBytesAllocated()!=totalVideoAllocated-totalVideoFreed)
			{
				UE_LOG(TrueSky,Warning,TEXT("Memory tracking issue"));
			}
#endif
			totalVideoAllocated	+=nbytes;
			int currentVideoAllocated=totalVideoAllocated-totalVideoFreed;
			if(currentVideoAllocated>maxVideoAllocated)
			{
				maxVideoAllocated=currentVideoAllocated;
#ifdef DEBUG
				if(maxVideoAllocated>(1024*1024*12))
					UE_LOG(TrueSky,Display,TEXT("Max video memory allocated %s"),*memstring(maxVideoAllocated));
#endif
			}
			memBlocks[ptr]	=blck;
			std::string name;
			if(fn&&strlen(fn)>0)
				name=fn;
			else
			{
				name="Unnamed";
			}
			{
				auto i=memoryTracks.find(name);
				if(i==memoryTracks.end())
				{
					memoryTracks[name]=0;
					i=memoryTracks.find(name);
				}
				else
				{
					if(i->second>0&&nbytes>(1024*1024*1))
						UE_LOG(TrueSky,Display,TEXT("Adding %s to existing memory allocation for %s"),*memstring(nbytes),ANSI_TO_TCHAR(name.c_str()));
				}
				i->second+=nbytes;
				allocationNames[ptr]=name;
			}
			//UE_LOG(TrueSky,Display,TEXT("Allocated %s for %s"),*memstring(nbytes),ANSI_TO_TCHAR(name.c_str()));
#ifdef DEBUG
			if(GetCurrentVideoBytesAllocated()!=totalVideoAllocated-totalVideoFreed)
			{
				UE_LOG(TrueSky,Warning,TEXT("Memory tracking issue"));
			}
#endif
		}
		else
		{
			UE_LOG(TrueSky,Error,TEXT("Failed to allocate GPU memory"));
		}
		return ptr;
	}
	//! De-allocate the memory at \param address (requires that this memory was allocated with Allocate()).
	virtual void DeallocateVideoMemory(void* ptr)
	{
		if(ptr)
		{
			auto m=memBlocks.find(ptr);
			if(m==memBlocks.end())
			{
				UE_LOG(TrueSky,Warning,TEXT("Trying to deallocate memory that's not been allocated: %d"),(int64)ptr);
				return;
			}
			int size=m->second.Size;
			totalVideoFreed+=size;
			FMemBlock::Free(m->second);
			memBlocks.erase(m);
			auto n=allocationNames.find(ptr);
			if(n!=allocationNames.end())
			{
				const std::string &name=n->second;
				//UE_LOG(TrueSky,Display,TEXT("Freed %s for %s"),*memstring(size),ANSI_TO_TCHAR(name.c_str()));
				if(name.length())
				{
					auto i=memoryTracks.find(name);
					if(i!=memoryTracks.end())
					{
						i->second-=size;
						if(size>(1024*1024*1))
						{
							UE_LOG(TrueSky,Display,TEXT("Deallocating %s from %s"),*memstring(size),ANSI_TO_TCHAR(name.c_str()));
							if(i->second <= 0)
								allocationNames.erase(n);
						}
					}
				}
			}
#ifdef DEBUG
			if(GetCurrentVideoBytesAllocated()!=totalVideoAllocated-totalVideoFreed)
			{
				UE_LOG(TrueSky,Warning,TEXT("Memory tracking issue"));
			}
#endif
		}
	}

	const char *GetNameAtIndex(int index) const override
	{
		std::map<std::string,int>::const_iterator i=memoryTracks.begin();
		for(int j=0;j<index&&i!=memoryTracks.end();j++)
		{
			i++;
		}
		if(i==memoryTracks.end())
			return nullptr;
		return i->first.c_str();
	}
	int GetBytesAllocated(const char *name) const override
	{
		if(!name)
			return 0;
		auto i=memoryTracks.find(name);
		if(i==memoryTracks.end())
			return 0;
		return i->second;
	}
	int GetTotalBytesAllocated() const
	{
		return 0;
	}
	virtual int GetTotalVideoBytesAllocated() const
	{
		return totalVideoAllocated;
	}
	virtual int GetTotalVideoBytesFreed() const
	{
		return totalVideoFreed;
	}
	virtual int GetCurrentVideoBytesAllocated() const
	{
		int bytes=0;
		for(auto i:memoryTracks)
		{
			bytes+=i.second;
		}
		return bytes;
	}
	virtual void TrackVideoMemory(void* ,int ,const char *){}
	virtual void UntrackVideoMemory(void* ){}
};

MemoryAllocator ps4MemoryAllocator;
#endif

simul::base::MemoryInterface *simul::ue4::getMemoryInterface()
{
#if PLATFORM_PS4
	return &ps4MemoryAllocator;
#else
	return nullptr;
#endif
}
