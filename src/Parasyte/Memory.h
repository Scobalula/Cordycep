#pragma once
#include "MemoryPool.h"
#include <list>

namespace ps
{
	// A class to handle memory for tiny objects that must maintain their memory location.
	class Memory
	{
	private:
		// The size of the elements.
		size_t ElementSize;
		// The size of each pool
		size_t PoolSize;
		// The memory pools.
		std::list<MemoryPool> MemoryPools;

	public:
		// Creates new memory with the provided size.
		Memory(size_t elemSize, size_t poolSize);

		// Gets a memory pool with free slots.
		ps::MemoryPool& GetFreePool();
		// Allocates data from the memory pool.
		void* Allocate(size_t elemSize);
		// Deallocates data from the memory pool.
		void Deallocate(void* elem);
		// Frees empty pools.
		void FreeEmptyPools();
	};
}
