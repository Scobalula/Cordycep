#pragma once
#include "FastFileFlags.h"

namespace ps
{
    // The number of fast file blocks.
    constexpr size_t FastFileBlockCount = 32;

    // A class to hold a fast file memory block
    class FastFileMemoryBlock
    {
    public:
        // Block Memory
        void* Memory;
        // Size of the memory block
        size_t MemorySize;

        // Initializes Memory Block
        FastFileMemoryBlock()
        {
            Memory = nullptr;
            MemorySize = 0;
        }
        // Initializes the memory with the given size
        void Initialize(size_t memorySize, size_t alignment)
        {
            _aligned_free(Memory);

            if (memorySize > 0)
            {
                Memory = _aligned_malloc(memorySize, alignment);

                if (Memory != nullptr)
                {
                    std::memset(Memory, 0, memorySize);
                }
            }
            else
            {
                Memory = nullptr;
            }

            MemorySize = memorySize;
        }
        // Frees fast file memory.
        ~FastFileMemoryBlock()
        {
            _aligned_free(Memory);
        }
    };

    // A class to hold a loaded fast file.
    class FastFile
    {
    public:
        // The name of the fast file
        std::string Name;
        // The fast file ID.
        uint64_t ID;
        // The fast file parent.
        FastFile* Parent;
        // Whether or not this fast file is a common fast file.
        // TODO: Used ever in: ps::GameHandler::UnloadNonCommonFastFiles(), but unused for now.
        bool Common;
        // The fast file memory blocks.
        FastFileMemoryBlock MemoryBlocks[FastFileBlockCount];
        // Asset List (variable length depending on game)
        char AssetList[512];
        // Flags stored within this fast file relating to how it was loaded, etc.
        BitFlags<FastFileFlags> Flags;

        FastFile(std::string name);
    };
}
