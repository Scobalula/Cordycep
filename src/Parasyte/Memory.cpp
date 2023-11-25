#include "pch.h"
#include "Memory.h"

ps::Memory::Memory(size_t elemSize, size_t poolSize)
{
	ElementSize = elemSize;
	PoolSize = poolSize;
}

ps::MemoryPool& ps::Memory::GetFreePool()
{
	for (auto& pool : MemoryPools)
	{
		if (pool.HasFreeSlot())
		{
			return pool;
		}
	}

	// Scaling up count to avoid future pool creations.
	return MemoryPools.emplace_back(ElementSize, PoolSize * (MemoryPools.size() + 1));
}

void* ps::Memory::Allocate(size_t elemSize)
{
	auto& pool = GetFreePool();

	return pool.Allocate(elemSize);
}

void ps::Memory::Deallocate(void* elem)
{
	if (elem != nullptr)
	{
		ps::MemoryPool* pool = *(ps::MemoryPool**)((char*)elem - 16);
		pool->Deallocate(elem);
	}
}

void ps::Memory::FreeEmptyPools()
{
	for (std::list<ps::MemoryPool>::iterator i = MemoryPools.begin(); i != MemoryPools.end(); )
	{
		if (i->IsEmpty())
		{
			i = MemoryPools.erase(i);
		}
		else
		{
			i++;
		}
	}
}
