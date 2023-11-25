#pragma once
#include "Memory.h"
#include "XAsset.h"

namespace ps
{
	// A class to hold a pool of assets
	class XAssetPool
	{
	public:
		// The start of the asset chain.
		XAsset* Root;
		// The end of the asset chain.
		XAsset* End;
		// The asset hash table for this pool.
		std::unique_ptr<std::map<uint64_t, XAsset*>> LookupTable;
		// Storage for asset headers for this pool.
		std::unique_ptr<ps::Memory> HeaderMemory;
		// Storage for asset entries for this pool.
		std::unique_ptr<ps::Memory> AssetMemory;

		XAssetPool();
		XAssetPool(const size_t elemSize, const size_t poolSize);

		~XAssetPool();

		void Initialize(const size_t elemSize, const size_t poolSize);

		XAsset* CreateEntry(const char* name, const size_t len, const size_t type, const size_t headerSize, FastFile* owner, uint8_t* header, bool temp);

		XAsset* CreateEntry(const uint64_t hash, const size_t type, const size_t headerSize, FastFile* owner, uint8_t* header, bool temp);

		// Links an xasset entry, linking it to others and performing any required overrides/appends.
		XAsset* LinkXAssetEntry(const char* name, const size_t type, const size_t size, const bool temp, uint8_t* header, ps::FastFile* owner);
		// Attempts to locate the asset entry by name.
		XAsset* FindXAssetEntry(const char* name, const size_t len, const size_t type) const;
		// Attempts to locate the asset entry by hash.
		XAsset* FindXAssetEntry(const uint64_t hash, const size_t type) const;

		/// <summary>
		/// Clears assets loaded by the provided fast file.
		/// </summary>
		/// <param name="fastFile"></param>
		void ClearFastFileAssets(FastFile* fastFile);

		// Enumerates through all the entries with the given routine.
		void EnumerateEntries(std::function<void(XAsset*)> func);
	};
}


