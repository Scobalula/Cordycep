#include "pch.h"
#include "MemoryPool.h"

ps::MemoryPool::MemoryPool(const size_t elemSize, const size_t poolSize)
{
	ElementSize = elemSize + 16;
	PoolSize    = poolSize;
	BufferSize  = ElementSize * PoolSize;
	Buffer      = std::make_unique<char[]>(BufferSize);
	Allocations = 0;
	Time        = time(NULL);

	for (size_t i = 0; i < PoolSize - 1; i++)
	{
		*(uintptr_t*)(Buffer.get() + (i * ElementSize) + 0x00) = (uintptr_t)this;
		*(uintptr_t*)(Buffer.get() + (i * ElementSize) + 0x08) = (uintptr_t)Time;
		*(uintptr_t*)(Buffer.get() + (i * ElementSize) + 0x10) = (uintptr_t)(Buffer.get() + ((i + 1) * ElementSize) + 16);
	}

	FreeSlot = Buffer.get() + 16;
}

void* ps::MemoryPool::Allocate(const size_t elemSize)
{
	auto freeSlot = FreeSlot;
	*(uintptr_t*)&FreeSlot = (uintptr_t)(*(uintptr_t**)FreeSlot);
	Allocations++;
	return freeSlot;
}

void ps::MemoryPool::Deallocate(void* elem)
{
	std::memset(elem, 0, ElementSize - 16);

	*(uintptr_t*)elem = *(uintptr_t*)FreeSlot;
	*(uintptr_t*)&FreeSlot = (uintptr_t)elem;
	Allocations--;
}

const bool ps::MemoryPool::HasFreeSlot() const
{
	return *(uintptr_t*)FreeSlot != 0;
}

const size_t ps::MemoryPool::AllocationCount() const
{
	return Allocations;
}

const bool ps::MemoryPool::IsEmpty() const
{
	return AllocationCount() == 0;
}

const time_t ps::MemoryPool::GetCreationTime() const
{
	return Time;
}

