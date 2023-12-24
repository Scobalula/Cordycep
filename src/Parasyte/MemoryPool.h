#pragma once

namespace ps
{
	class MemoryPool
	{
	private:
		// The buffer.
		std::unique_ptr<char[]> Buffer;
		// The size of the elements.
		size_t ElementSize;
		// The size of each pool
		size_t PoolSize;
		// The size of the buffer.
		size_t BufferSize;
		// The number of allocations.
		size_t Allocations;
		// The first free slot
		char* FreeSlot;
		// The time this pool was created at.
		time_t Time;

	public:
		// Creates a new memory pool.
		MemoryPool(const size_t elemSize, const size_t poolSize);

		// Allocates the provided element.
		void* Allocate(const size_t elemSize);
		// Deallocates data from the memory pool.
		void Deallocate(void* elem);
		// Gets the free slot.
		bool HasFreeSlot() const;
		// Gets the number of allocations.
		const size_t AllocationCount() const;
		// Returns if this pool is empty.
		bool IsEmpty() const;
		// Gets the time this pool was created at.
		const time_t GetCreationTime() const;
	};
}