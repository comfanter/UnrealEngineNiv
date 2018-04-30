
#pragma once
#include <cstddef>

namespace simul
{
	namespace base
	{
		class MemoryInterface
		{
		public:
			void* Allocate(size_t nbytes)
			{
				return AllocateTracked(nbytes,1,nullptr);
			}
			void* Allocate(size_t nbytes,size_t align)
			{
				return AllocateTracked(nbytes,align,nullptr);
			}
			void* AllocateVideoMemory(size_t nbytes,size_t align)
			{
				return AllocateVideoMemoryTracked(nbytes,align,nullptr);
			}
			virtual void* AllocateTracked(size_t /*nbytes*/,size_t /*align*/,const char * /*fn_name*/){ return nullptr; }
			virtual void* AllocateVideoMemoryTracked(size_t /*nbytes*/,size_t /*align*/,const char * /*fn_name*/){ return nullptr; }
			virtual void Deallocate(void* address) = 0;
			virtual void DeallocateVideoMemory(void* address) = 0;

			virtual const char *GetNameAtIndex(int index) const=0;
			virtual int GetBytesAllocated(const char *name) const=0;
			virtual int GetTotalBytesAllocated() const=0;
			virtual int GetCurrentVideoBytesAllocated() const=0;
			virtual int GetTotalVideoBytesAllocated() const=0;
			virtual int GetTotalVideoBytesFreed() const=0;
			virtual void TrackVideoMemory(void* ,int ,const char *){}
			virtual void UntrackVideoMemory(void* ){}
		};
	}
	namespace ue4
	{
		base::MemoryInterface *getMemoryInterface();
	}
}