#pragma once
#include "Pattern.h"

namespace ps
{
	// The strategy to use when resolving scanned values.
	enum class ScanType
	{
		// The scan lands us where we need to be and no resolving needs to be done.
		NoResolving,
		// The data we are reading is relative to the start of the module.
		FromModuleBegin,
		// We're resolving from a compare that is 7 bytes long where the offset is 2 bytes from the instruction.
		FromCmpToAddress,
		// We're resolving from data that is 4 bytes long where the offset is relative to after the data.
		FromEndOfData,
		// We're resolving from data that is 4 bytes long where the offset is relative to a byte after the data.
		FromEndOfByteCmp,
	};

	// A class to hold a game module.
	class GameModule
	{
	private: 
		// A map of cached offsets to pattern hashes
		std::map<uint64_t, std::vector<uintptr_t>> CachedOffsets;
	public:
		// The native game module in memory.
		HMODULE Handle;
		// The size of the game module.
		void* CodeSegment;
		size_t CodeSize;
		// The XXHash checksum of the game code.
		uint64_t Checksum;


		// Whether or not the module is loaded.
		bool Loaded;

		/// Creates a new empty game module.
		GameModule() : Handle(NULL), CodeSegment(NULL), CodeSize(0), Checksum(0), Loaded(false) { }


		// Loads the module from the given file path.
		bool Load(const std::string& filePath);

		// Frees the module.
		bool Free();

		// Calculates the module checksum using XXHash.
		uint64_t CalculateModuleCodeChecksum();

		// Attempts to get the cached offset for the provided id.
		char* GetCachedOffset(uint32_t id);

		// Resolves the data by the given type.
		char* Resolve(char* result, ScanType type);

		char* FindVariableAddress(const Pattern& pattern, size_t offsetToData, std::string name, ScanType type);

		bool FindVariableAddress(void* variable, const Pattern& pattern, size_t offsetToData, std::string name, ScanType type);

		bool NullifyFunction(const Pattern& pattern, size_t offsetFromSig, const std::string& name, bool multiple, bool relative);

		bool PatchBytes(const Pattern& pattern, size_t offsetFromSig, std::string name, PBYTE data, size_t dataLen, bool multiple, bool relative);

		// Creates a detour at the current location.
		bool CreateDetour(uintptr_t source, uintptr_t destination);
		// Creates a detour at the current location.
		bool CreateDetourEx(uintptr_t* source, uintptr_t destination);

		// Scans the module for the provided signature.
		std::vector<char*> Scan(const Pattern& pattern, bool multiple);

		// Cache functions
		const void LoadCache(const std::string& cachePath);
		const void SaveCache(const std::string& cachePath) const;

		// Parse and fix the IAT of the game handle. (is called in GameModule.Load())
		void FixIAT(HINSTANCE handle);

		// Disposes of the module.
		~GameModule()
		{
			Free();
		}
	};
}
