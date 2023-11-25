#include "pch.h"
#if _WIN64
#include "Parasyte.h"
#include "CoDMW4Handler.h"
#include "OodleDecompressorV1.h"

// Internal Functions
namespace ps::CoDMW4Internal
{
	// The fast file decompressor.
	std::unique_ptr<Decompressor> FFDecompressor;
	// The patch file decompressor.
	std::unique_ptr<Decompressor> FPDecompressor;

	// Resolves the stream position from within the game.
	__int64(__cdecl* ResolveStreamPositionOriginal)(__int64* a1) = nullptr;
	// Initializes the patch function info.
	uint64_t(__fastcall* InitializePatch)();
	// Requests data from the patch file.
	void* (__fastcall* RequestPatchFileData)();
	// Requests data from the fast file.
	void* (__fastcall* RequestFastFileData)();
	// Requests data from the final file buffer.
	void* (__fastcall* RequestFinalFileData)();
	// Function that handles requesting and resolving patch data
	uint64_t(__cdecl* RequestPatchedData)(void* a, void* b, void* c, void* d, void* e);
	// Decrypts a string.
	char* (__cdecl* DecrpytString)(char* input);
	// Parses a fast file and all data within it.
	void* (__cdecl* ParseFastFile)(const void* a, const char* b, const char* c, bool d);
	// Assigns fast file memory pointers.
	void(__cdecl* AssignFastFileMemoryPointers)(void* blocks);
	// Adds an asset offset to the list.
	void(__cdecl* AddAssetOffset)(size_t* assetType);
	// Initializes Asset Alignment.
	void(__cdecl* InitAssetAlignmentInternal)();

	// Zone Loader Flag (must be 1)
	uint8_t* ZoneLoaderFlag = nullptr;
	// The size of the below buffer
	size_t XAssetAlignmentBufferSize = 65535 * 32;
	// A list of offsets for each buffer
	uint64_t* XAssetOffsetList = nullptr;
	// A buffer for asset alignment.
	std::unique_ptr<uint8_t[]> XAssetAlignmentBuffer = nullptr;
	// A buffer for patch file.
	std::unique_ptr<uint8_t[]> PatchFileState = nullptr;
	// XFile Versions
	int* XFileVersions = nullptr;
	// Pool Sizes
	int* XAssetHeaderSizes = nullptr;

	// Resolves the stream position, running bounds checks.
	__int64 __fastcall ResolveStreamPosition(__int64* a1)
	{
		size_t zoneIndex = (*a1 >> 32) & 0xF;
		size_t zoneOffset = (size_t)((uint32_t)*a1 - 1);

		// A very basic check to see if this offset is
		// outside the bounds of the zone buffers.
		// Seems to work pretty well to avoid crashing for invalid files
		// from previous updates.
		if (zoneIndex >= 11)
			throw std::exception("This file is from a previous update, skipping.");
		if (zoneOffset >= ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[zoneIndex].MemorySize)
			throw std::exception("This file is from a previous update, skipping.");

		return ResolveStreamPositionOriginal(a1);
	}
	// Reads patch data from the patch file.
	void ReadPatchFile(void* state, void* ptr, size_t size)
	{
		FPDecompressor->Read(ptr, size, 0);
	}
	// Reads patch data from the fast file.
	void ReadFastFile(void* state, void* ptr, size_t size)
	{
		FFDecompressor->Read(ptr, size, 0);
	}
	// Initializes Asset Alignment.
	void InitAssetAlignment()
	{
		// Seems to store offsets to assets and some value used for alignment
		// seems to tie into their patching system so they can keep relative
		// pointers the same?

		std::memset(XAssetAlignmentBuffer.get(), 0, XAssetAlignmentBufferSize);

		for (size_t i = 0; i < 16; i++)
		{
			if (XAssetOffsetList[i] > 0)
			{
				XAssetOffsetList[i] = (uint64_t)(XAssetAlignmentBuffer.get() + 65535 * i);
			}
		}
	}
	// Reads data from the Fast File. If patching is enabled, then data is consumed from the patch and fast file.
	void ReadXFile(char* pos, size_t size)
	{
		// We have a patch file included
		if (FPDecompressor->IsValid() && PatchFileState != nullptr && *(uint64_t*)(PatchFileState.get() + 384) > 0)
		{
			size_t current = *(uint32_t*)(PatchFileState.get() + 328);
			size_t remaining = size;

			while (remaining > 0)
			{
				// We need to check if we need more data from the patch/fast file.
				if (*(uint32_t*)(PatchFileState.get() + 312) == current)
				{
					*(uint64_t*)(PatchFileState.get() + 328) = 0;

					if (!RequestPatchedData(
						PatchFileState.get(),
						PatchFileState.get() + 232,
						RequestFastFileData,
						RequestPatchFileData,
						RequestFinalFileData))
					{
						ps::log::Log(ps::LogType::Error, "Failed to patch fast file, error code: %lli", *(uint32_t*)(PatchFileState.get() + 380));
						throw std::exception("MW5_PatchFile_RequestData(...) failed");
					}
				}

				size_t size = (size_t)(*(uint32_t*)(PatchFileState.get() + 312) - *(uint32_t*)(PatchFileState.get() + 328));

				if (remaining < size)
					size = remaining;

				std::memcpy(pos, (void*)(*(uint64_t*)(PatchFileState.get() + 296) + *(uint64_t*)(PatchFileState.get() + 328)), size);
				*(uint32_t*)(PatchFileState.get() + 328) += (uint32_t)size;
				pos += size;
				remaining -= size;

				current = *(uint32_t*)(PatchFileState.get() + 328);
			}
		}
		else
		{
			FFDecompressor->Read(pos, size, 0);
		}
	}
	// Resets patch file structure.
	void ResetPatchState(size_t headerDataSize, size_t headerFastFileDataSize, size_t headerPatchFileDataSize, size_t headerDecompressedSize)
	{
		// Check if we need to initialize the data buffer
		if (PatchFileState == nullptr && headerDataSize != 0 && headerFastFileDataSize != 0 && headerPatchFileDataSize != 0)
			PatchFileState = std::make_unique<uint8_t[]>(65535);

		if (PatchFileState != nullptr)
		{
			if (*(void**)(PatchFileState.get() + 296) != nullptr)
				_aligned_free(*(void**)(PatchFileState.get() + 296));
			if (*(void**)(PatchFileState.get() + 264) != nullptr)
				_aligned_free(*(void**)(PatchFileState.get() + 264));
			if (*(void**)(PatchFileState.get() + 336) != nullptr)
				_aligned_free(*(void**)(PatchFileState.get() + 336));

			*(void**)(PatchFileState.get() + 296) = nullptr;
			*(void**)(PatchFileState.get() + 264) = nullptr;
			*(void**)(PatchFileState.get() + 336) = nullptr;
		}

		// If we're passing 0, we're not allocating, just freeing.
		if (headerDataSize != 0 && headerFastFileDataSize != 0 && headerPatchFileDataSize != 0)
		{
			std::memset(PatchFileState.get(), 0, 65535);
			InitializePatch();

			// Align our data sizes
			size_t dataSize = (headerDataSize + 4095) & 0xFFFFFFFFFFFFF000;
			if (dataSize < 0x10000)
				dataSize = 0x10000;
			size_t fastFileDataSize = (headerFastFileDataSize + 4095) & 0xFFFFFFFFFFFFF000;
			if (fastFileDataSize < 0x10000)
				fastFileDataSize = 0x10000;
			size_t patchFileDataSize = (headerPatchFileDataSize + 4095) & 0xFFFFFFFFFFFFF000;
			if (patchFileDataSize < 0x10000)
				patchFileDataSize = 0x10000;

			*(void**)(PatchFileState.get() + 296) = _aligned_malloc(dataSize, 4096);
			*(void**)(PatchFileState.get() + 264) = _aligned_malloc(fastFileDataSize, 4096);
			*(void**)(PatchFileState.get() + 336) = _aligned_malloc(patchFileDataSize, 4096);
		}

		if (PatchFileState != nullptr)
			*(uint64_t*)(PatchFileState.get() + 384) = headerDecompressedSize;
	}
	// Calls memset.
	void* Memset(void* ptr, size_t val, size_t size)
	{
		return std::memset(ptr, (int)val, size);
	}
	// Allocates a unique string entry.
	size_t AllocateUniqueString(char* str, int type)
	{
		auto strLen = strlen(str) + 1;
		auto id = XXHash64::hash(str, strLen, 0);
		auto potentialEntry = ps::Parasyte::GetCurrentHandler()->StringLookupTable->find(id);

		if (potentialEntry != ps::Parasyte::GetCurrentHandler()->StringLookupTable->end())
		{
			return (int)potentialEntry->second;
		}

		auto offset = ps::Parasyte::GetCurrentHandler()->StringPoolSize;
		std::memcpy(&ps::Parasyte::GetCurrentHandler()->Strings[offset], str, strLen);
		ps::Parasyte::GetCurrentHandler()->StringPoolSize += strLen;
		ps::Parasyte::GetCurrentHandler()->StringLookupTable->operator[](id) = offset;

		return offset;
	}

	// Links an xasset of the given type.
	void* LinkGenericXAsset(uint32_t assetType, char*** asset)
	{
		auto pool     = &ps::Parasyte::GetCurrentHandler()->XAssetPools[assetType];
		auto temp     = ***asset == ',';
		auto name     = **asset;
		auto size     = XAssetHeaderSizes[assetType];
		auto capacity = size > 1048576 ? 1 : (size_t)(8388608 / size);

		pool->Initialize(XAssetHeaderSizes[assetType], capacity);

		if (temp)
			std::memmove((void*)&name[0], &name[1], strlen(name));

		auto nameLen = strlen(**asset);
		auto result = pool->LinkXAssetEntry(name, assetType, size, temp, (uint8_t*)*asset, ps::Parasyte::GetCurrentFastFile());

		// If we're an image, we need to check if we want to allocate an image slot
		if (assetType == 19)
		{
			auto gfxImage = result->Header;
			auto imageData = *(uint8_t**)(gfxImage + 224);
			auto imageDataSize = (size_t)*(uint32_t*)(gfxImage + 28);

			if (imageData != nullptr && result->ExtendedData == nullptr)
			{
				result->ExtendedDataSize = imageDataSize;
				result->ExtendedData = std::make_unique<uint8_t[]>(result->ExtendedDataSize);
				result->ExtendedDataPtrOffset = 224;
				std::memcpy(result->ExtendedData.get(), imageData, result->ExtendedDataSize);
				*(uint8_t**)(gfxImage + 224) = result->ExtendedData.get();
				ps::log::Log(ps::LogType::Verbose, "Resolved loaded data for %s.", *asset, assetType);
			}
		}

		size_t toPop[2]{ assetType, (size_t)*asset };

		AddAssetOffset(toPop);

		// Loggary for Stiggary
		ps::log::Log(ps::LogType::Verbose, "%s Type: %i @ %llu.", **asset, assetType, (uint64_t)result->Header);

		return result->Header;
	}
	// Links a generic xasset.
	void* LinkGenericXAssetEx(uint32_t assetType, char*** asset)
	{
		return LinkGenericXAsset(assetType, asset);
	}
	// Links an xasset of the given type.
	void* LinkDlogSchemaAsset(char** a1)
	{
		return LinkGenericXAsset(115, &a1);
	}
}

const std::string ps::CoDMW4Handler::GetName()
{
	return "Call of Duty: Modern Warfare";
}

bool ps::CoDMW4Handler::Initialize(const std::string& gameDirectory)
{
	GameDirectory = gameDirectory;

	LoadConfigs("Data\\Configs\\CoDMW4Handler.json");
	SetConfig();
	CopyDependencies();
	OpenGameDirectory(GameDirectory);
	OpenGameModule(CurrentConfig->ModuleName);

	if (!ps::oodle::Initialize("Data\\Deps\\oo2core_7_win64.dll"))
	{
		ps::log::Log(ps::LogType::Error, "Failed to load the dll for Oodle Decompression.");
		return false;
	}

	Module.LoadCache(CurrentConfig->CacheName);

	ResolvePatterns();

	PS_SETGAMEVAR(ps::CoDMW4Internal::InitializePatch);
	PS_SETGAMEVAR(ps::CoDMW4Internal::RequestFastFileData);
	PS_SETGAMEVAR(ps::CoDMW4Internal::RequestPatchFileData);
	PS_SETGAMEVAR(ps::CoDMW4Internal::RequestFinalFileData);
	PS_SETGAMEVAR(ps::CoDMW4Internal::XFileVersions);
	PS_SETGAMEVAR(ps::CoDMW4Internal::XAssetHeaderSizes);
	PS_SETGAMEVAR(ps::CoDMW4Internal::RequestPatchedData);
	PS_SETGAMEVAR(ps::CoDMW4Internal::AssignFastFileMemoryPointers);
	PS_SETGAMEVAR(ps::CoDMW4Internal::InitAssetAlignmentInternal);
	PS_SETGAMEVAR(ps::CoDMW4Internal::AddAssetOffset);
	PS_SETGAMEVAR(ps::CoDMW4Internal::XAssetOffsetList);
	PS_SETGAMEVAR(ps::CoDMW4Internal::ZoneLoaderFlag);
	PS_SETGAMEVAR(ps::CoDMW4Internal::ParseFastFile);
	PS_SETGAMEVAR(ps::CoDMW4Internal::DecrpytString);
	PS_DETGAMEVAR(ps::CoDMW4Internal::ReadXFile);
	PS_DETGAMEVAR(ps::CoDMW4Internal::AllocateUniqueString);
	PS_DETGAMEVAR(ps::CoDMW4Internal::LinkGenericXAsset);
	PS_DETGAMEVAR(ps::CoDMW4Internal::ReadPatchFile);
	PS_DETGAMEVAR(ps::CoDMW4Internal::ReadFastFile);
	PS_DETGAMEVAR(ps::CoDMW4Internal::Memset);
	PS_INTGAMEVAR(ps::CoDMW4Internal::ResolveStreamPosition, ps::CoDMW4Internal::ResolveStreamPositionOriginal);

	ps::CoDMW4Internal::XAssetAlignmentBuffer = std::make_unique<uint8_t[]>(ps::CoDMW4Internal::XAssetAlignmentBufferSize);

	XAssetPoolCount   = 256;
	XAssetPools       = std::make_unique<XAssetPool[]>(XAssetPoolCount);
	Strings           = std::make_unique<char[]>(0x2000000);
	StringPoolSize    = 0;
	Initialized       = true;
	StringLookupTable = std::make_unique<std::map<uint64_t, size_t>>();

	Module.SaveCache(CurrentConfig->CacheName);
#if _DEBUG
	LoadAliases("F:\\Data\\VisualStudio\\Projects\\HydraX\\src\\HydraX\\bin\\x64\\Debug\\exported_files\\ModernWarfareAliases.json");
#else
	LoadAliases("Data\\ModernWarfareAliases.json");
#endif

	// ps::XAsset::XAssetMemory = std::make_unique<ps::Memory>();

	return true;
}

bool ps::CoDMW4Handler::Uninitialize()
{
	Module.Free();

	XAssetPoolCount       = 256;
	XAssetPools           = nullptr;
	Strings               = nullptr;
	StringPoolSize        = 0;
	Initialized           = false;
	StringLookupTable     = nullptr;
	FileSystem			  = nullptr;
	GameDirectory.clear();

	return true;
}

bool ps::CoDMW4Handler::IsValid(const std::string& param)
{
	return strcmp(param.c_str(), "mw4") == 0;
}


bool ps::CoDMW4Handler::ListFiles()
{
	FileSystem->EnumerateFiles(CurrentConfig->FilesDirectory, "*.ff", false, [](const std::string& name, const size_t size)
	{
		ps::log::Log(ps::LogType::Normal, "File: %s Available: 1", name.c_str());
	});

	return true;
}

bool ps::CoDMW4Handler::Exists(const std::string& ffName)
{
	return FileSystem->Exists(GetFileName(ffName));
}

bool ps::CoDMW4Handler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
{
	auto newFastFile = CreateUniqueFastFile(ffName);

	if (newFastFile == nullptr)
	{
		ps::log::Log(ps::LogType::Error, "The file: %s is already loaded.", ffName.c_str());
		return false;
	}

	newFastFile->Parent = parent;
	newFastFile->Flags = flags;

	// Set current ff for loading purposes.
	ps::Parasyte::PushTelemtry("LastFastFileName", ffName);
	ps::Parasyte::SetCurrentFastFile(newFastFile);

	ps::FileHandle ffHandle(FileSystem->OpenFile(GetFileName(ffName + ".ff"), "r"), FileSystem.get());
	ps::FileHandle fpHandle(FileSystem->OpenFile(GetFileName(ffName + ".fp"), "r"), FileSystem.get());

	if (!ffHandle.IsValid())
	{
		ps::log::Log(ps::LogType::Error, "The provided fast file: %s could not be found in the game's file system.", ffName.c_str());
		ps::log::Log(ps::LogType::Error, "Make sure any updates are finished and check for any content packages.");
		UnloadFastFile(ffName);
		return false;
	}

	uint8_t ffHeader[136]{};
	uint8_t fpHeader[328]{};

	ffHandle.Read(&ffHeader[0], 0, sizeof(ffHeader));

	// We must check if we have a patch file, we'll need to take a different
	// route if we do during file reading to ensure we're patching with new data.
	if (fpHandle.IsValid())
	{
		fpHandle.Read(&fpHeader[0], 0, sizeof(fpHeader));

		if (std::memcmp(&ffHeader[0], &fpHeader[56], sizeof(ffHeader)) != 0)
		{
			ps::log::Log(ps::LogType::Error, "Current fast file header does not match the one within the patch file.");
			ps::log::Log(ps::LogType::Error, "This may indicate your install is corrupt, or the file provided is for content no longer available.");
			return false;
		}

		// Patch the fast file header
		std::memcpy(&ffHeader[0], &fpHeader[192], sizeof(ffHeader));
	}

	ps::CoDMW4Internal::ResetPatchState(
		*(uint64_t*)&fpHeader[16],
		*(uint64_t*)&fpHeader[24],
		*(uint64_t*)&fpHeader[32],
		*(uint64_t*)&fpHeader[48]);

	// Verify XFile Version
	if (ps::CoDMW4Internal::XFileVersions[*(uint8_t*)(ffHeader + 18)] != *(uint32_t*)(ffHeader + 12))
	{
		ps::log::Log(ps::LogType::Error, "Invalid XFile Version, got version: 0x%llx expected: 0x%llx", *(uint32_t*)(ffHeader + 12), ps::CoDMW4Internal::XFileVersions[*(uint8_t*)(ffHeader + 18)]);
		return false;
	}

	uint64_t* bufferSizes = (uint64_t*)&ffHeader[48];

	for (size_t i = 0; i < 11; i++)
	{
		if (bufferSizes[i] > 0)
		{
			ps::log::Log(ps::LogType::Verbose, "Allocating block: %llu of size: 0x%llx", i, bufferSizes[i]);
			ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[i].Initialize(bufferSizes[i], 4096);
		}
	}

	ps::CoDMW4Internal::FFDecompressor = std::make_unique<OodleDecompressorV1>(ffHandle, true);
	ps::CoDMW4Internal::FPDecompressor = std::make_unique<OodleDecompressorV1>(fpHandle, true);

	ps::CoDMW4Internal::InitializePatch();
	ps::CoDMW4Internal::InitAssetAlignment();
	ps::CoDMW4Internal::InitAssetAlignmentInternal();
	ps::CoDMW4Internal::AssignFastFileMemoryPointers(&ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[0]);
	ps::CoDMW4Internal::ZoneLoaderFlag[0] = 1;
	ps::CoDMW4Internal::ParseFastFile(nullptr, ps::Parasyte::GetCurrentFastFile()->AssetList, ffName.c_str(), 0);
	ps::CoDMW4Internal::FFDecompressor = nullptr;
	ps::CoDMW4Internal::FPDecompressor = nullptr;

	ps::log::Log(ps::LogType::Normal, "Successfully loaded: %s", ffName.c_str());

	// If we have no parent, we are a root, and need to attempt to load the localized, etc.
	if (newFastFile->Parent == nullptr && !flags.HasFlag(FastFileFlags::NoChildren))
	{
		auto techsetsName = "techsets_" + ffName;
		auto wwName = "worldwide\\ww_" + ffName;

		// Attempt to load the techsets file, this usually contains
		// materials, shaders, technique sets, etc.
		if (Exists(techsetsName) && !IsFastFileLoaded(techsetsName))
			LoadFastFile(techsetsName, newFastFile, flags);
		// Attempt to load the ww file, same as locale, not as important
		// but we'll still load it, as it can contain data we need.
		if (Exists(wwName) && !IsFastFileLoaded(wwName))
			LoadFastFile(wwName, newFastFile, flags);

		// Check for locale prefix
		if (RegionPrefix.size() > 0)
		{
			auto localeName = RegionPrefix + ffName;

			// Now load the locale, not as important, as usually
			// only contains localized data.
			if (Exists(localeName) && !IsFastFileLoaded(localeName))
				LoadFastFile(localeName, newFastFile, flags);
		}
	}

	return true;
}

bool ps::CoDMW4Handler::CleanUp()
{
	ps::CoDMW4Internal::FFDecompressor = nullptr;
	ps::CoDMW4Internal::FPDecompressor = nullptr;
	ps::CoDMW4Internal::ResetPatchState(0, 0, 0, 0);
	return false;
}

const std::string ps::CoDMW4Handler::GetShorthand()
{
	return "mw4";
}

PS_CINIT(ps::GameHandler, ps::CoDMW4Handler, ps::GameHandler::GetHandlers());
#endif