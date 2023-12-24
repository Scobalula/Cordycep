#include "pch.h"
#if _WIN64
#include "Parasyte.h"
#include "CoDVGHandler.h"
#include "OodleDecompressorV1.h"

// Internal Functions
namespace ps::CoDVGInternal
{
	// The fast file decompressor.
	std::unique_ptr<Decompressor> FFDecompressor;
	// The patch file decompressor.
	std::unique_ptr<Decompressor> FPDecompressor;

	// Resolves the stream position from within the game.
	__int64(__cdecl* ResolveStreamPosition)(__int64* a1) = nullptr;
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
	char* (__cdecl* DecrpytString)(char* a, size_t b, char* c);
	// Parses a fast file and all data within it.
	void* (__cdecl* ParseFastFile)(const void* a, const char* b, const char* c, bool d);
	// Assigns fast file memory pointers.
	void(__cdecl* AssignFastFileMemoryPointers)(void* blocks);
	// Adds an asset offset to the list.
	void(__cdecl* AddAssetOffset)(size_t* assetType);
	// Initializes Asset Alignment.
	void(__cdecl* InitAssetAlignmentInternal)();
	// Gets the header size for the given type.
	const size_t(__fastcall* GetXAssetTypeSize)(uint32_t xassetType);

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
	// A buffer for loading strings.
	std::unique_ptr<char[]> StrDecryptBuffer = nullptr;
	// The size of the above string buffer.
	constexpr size_t StrDecryptBufferSize = 65535;

	// Resolves the stream position, running bounds checks.
	__int64 __fastcall ResolveStreamPositionEx(__int64* a1)
	{
		size_t zoneIndex = (*a1 >> 32) & 0xF;
		size_t zoneOffset = *a1 - 1;

		// A very basic check to see if this offset is
		// outside the bounds of the zone buffers.
		// Seems to work pretty well to avoid crashing for invalid files
		// from previous updates.
		if (zoneIndex >= 11)
			throw std::exception("This file is from a previous update, skipping.");
		if (zoneOffset >= ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[zoneIndex].MemorySize)
			throw std::exception("This file is from a previous update, skipping.");

		return ResolveStreamPosition(a1);
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
		if (FPDecompressor->IsValid() && *(uint64_t*)(PatchFileState.get() + 384) > 0)
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
						throw std::exception("VG_PatchFile_RequestData(...) failed");
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
		char* decrypted = str;

		// Check if the string is actually encrypted.
		if ((*str & 0xC0) == 0x80)
		{
			decrypted = DecrpytString(StrDecryptBuffer.get(), StrDecryptBufferSize, str);
		}

		auto strLen = strlen(decrypted) + 1;
		auto id = XXHash64::hash(decrypted, strLen, 0);
		auto potentialEntry = ps::Parasyte::GetCurrentHandler()->StringLookupTable->find(id);

		if (potentialEntry != ps::Parasyte::GetCurrentHandler()->StringLookupTable->end())
		{
			return potentialEntry->second;
		}

		auto offset = ps::Parasyte::GetCurrentHandler()->StringPoolSize;
		std::memcpy(&ps::Parasyte::GetCurrentHandler()->Strings[offset], decrypted, strLen);

		ps::Parasyte::GetCurrentHandler()->StringPoolSize += strLen;
		ps::Parasyte::GetCurrentHandler()->StringLookupTable->operator[](id) = offset;

		return offset;
	}

	// Links an xasset of the given type.
	void* LinkGenericXAsset(uint32_t assetType, char*** asset)
	{
		auto pool = &ps::Parasyte::GetCurrentHandler()->XAssetPools[assetType];
		auto temp = ***asset == ',';
		auto name = **asset;
		auto size = GetXAssetTypeSize(assetType);

		pool->Initialize(size, 16384);

		if (temp)
		{
			std::memmove((void*)&name[0], &name[1], strlen(name));
		}

		auto nameLen = strlen(**asset);

		// Our result
		auto result = pool->FindXAssetEntry(**asset, nameLen, assetType);

		// We need to check if we have an existing asset to override
		// If we have, we need to override or append, maintaining same pointer
		// to the address of the header so that if we unload this ff, etc.
		// the pointers from other assets are maintained
		if (result != nullptr)
		{
			if (temp)
			{
				result->AppendChild(
					ps::Parasyte::GetCurrentFastFile(),
					(uint8_t*)*asset,
					temp);
			}
			else
			{
				result->Override(
					ps::Parasyte::GetCurrentFastFile(),
					(uint8_t*)*asset,
					temp);
			}
		}

		// If we got to this point, we haven't found the entry, so let's create it
		if (result == nullptr)
		{
			result = pool->CreateEntry(
				**asset,
				nameLen,
				assetType,
				size,
				ps::Parasyte::GetCurrentFastFile(),
				(uint8_t*)*asset,
				temp);
		}

		// If we're an image, we need to check if we want to allocate an image slot
		//if (assetType == 19)
		//{
		//	auto gfxImage = (MW4GfxImage*)result->Header;

		//	if (gfxImage->LoadedImagePointer != nullptr && result->ExtendedData == nullptr)
		//	{
		//		result->ExtendedDataSize = gfxImage->BufferSize;
		//		result->ExtendedData = std::make_unique<uint8_t[]>(result->ExtendedDataSize);
		//		result->ExtendedDataPtrOffset = offsetof(MW4GfxImage, LoadedImagePointer);
		//		std::memcpy(result->ExtendedData.get(), gfxImage->LoadedImagePointer, result->ExtendedDataSize);
		//		gfxImage->LoadedImagePointer = result->ExtendedData.get();
		//		ps::log::Log(ps::LogType::Verbose, "Resolved loaded data for %s.", *asset, assetType);
		//	}
		//}
// #if PRIVATE_GRAM_GRAM
		//if (assetType == 41 && ModernWarfare_Str_Decrypt != nullptr)
		//{
		//	*(uint64_t*)(result->Header + 8) = (uint64_t)ModernWarfare_Str_Decrypt(*(char**)(result->Header + 8));
		//}
// #else
// 		if (assetType == 41)
// 		{
// 			char* name = *(char**)(result->Header);
// 			std::memset(name, 0, strlen(name));
// 			char* str = *(char**)(result->Header + 8);
// 			std::memset(str, 0, strlen(str));
// 		}
// #endif
		size_t toPop[2]{ assetType, (size_t)*asset };

		AddAssetOffset(toPop);

		// Loggary for Stiggary
		// ps::log::Log(ps::LogType::Verbose, "%s Type: %i @ %llu.", asset->Header->Name, assetType, (uint64_t)result->Header);

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

const std::string ps::CoDVGHandler::GetName()
{
	return "Call of Duty: Vanguard";
}

bool ps::CoDVGHandler::Initialize(const std::string& gameDirectory)
{
	Configs.clear();
	GameDirectory = gameDirectory;

	// LoadConfigs("Data\\Configs\\CoDVGHandler.json");
	LoadConfigs("CoDVGHandler.toml");
	SetConfig();
	CopyDependencies();
	OpenGameDirectory(GameDirectory);
	OpenGameModule(CurrentConfig->ModuleName);

	if (!ps::oodle::Initialize("Data\\Deps\\oo2core_8_win64.dll"))
	{
		ps::log::Log(ps::LogType::Error, "Failed to load the dll for Oodle Decompression.");
		return false;
	}

	Module.LoadCache(CurrentConfig->CacheName);

	ResolvePatterns();

	PS_SETGAMEVAR(ps::CoDVGInternal::InitializePatch);
	PS_SETGAMEVAR(ps::CoDVGInternal::RequestPatchedData);
	PS_SETGAMEVAR(ps::CoDVGInternal::RequestFastFileData);
	PS_SETGAMEVAR(ps::CoDVGInternal::RequestPatchFileData);
	PS_SETGAMEVAR(ps::CoDVGInternal::RequestFinalFileData);
	PS_SETGAMEVAR(ps::CoDVGInternal::XFileVersions);
	PS_SETGAMEVAR(ps::CoDVGInternal::GetXAssetTypeSize);
	PS_SETGAMEVAR(ps::CoDVGInternal::AssignFastFileMemoryPointers);
	PS_SETGAMEVAR(ps::CoDVGInternal::InitAssetAlignmentInternal);
	PS_SETGAMEVAR(ps::CoDVGInternal::AddAssetOffset);
	PS_SETGAMEVAR(ps::CoDVGInternal::XAssetOffsetList);
	PS_SETGAMEVAR(ps::CoDVGInternal::ZoneLoaderFlag);
	PS_SETGAMEVAR(ps::CoDVGInternal::ParseFastFile);
	PS_SETGAMEVAR(ps::CoDVGInternal::DecrpytString);
	PS_DETGAMEVAR(ps::CoDVGInternal::ReadXFile);
	PS_DETGAMEVAR(ps::CoDVGInternal::AllocateUniqueString);
	PS_DETGAMEVAR(ps::CoDVGInternal::LinkGenericXAsset);
	PS_DETGAMEVAR(ps::CoDVGInternal::ReadFastFile);
	PS_DETGAMEVAR(ps::CoDVGInternal::ReadPatchFile);

	XAssetPoolCount   = 256;
	XAssetPools       = std::make_unique<XAssetPool[]>(XAssetPoolCount);
	Strings           = std::make_unique<char[]>(0x2000000);
	StringPoolSize    = 0;
	Initialized       = true;
	StringLookupTable = std::make_unique<std::map<uint64_t, size_t>>();

	Module.SaveCache(CurrentConfig->CacheName);
	LoadAliases(CurrentConfig->AliasesName);

	// Game specific buffers.
	ps::CoDVGInternal::XAssetAlignmentBuffer = std::make_unique<uint8_t[]>(ps::CoDVGInternal::XAssetAlignmentBufferSize);
	ps::CoDVGInternal::StrDecryptBuffer = std::make_unique<char[]>(ps::CoDVGInternal::StrDecryptBufferSize);

	return true;
}

bool ps::CoDVGHandler::Deinitialize()
{
	Module.Free();

	XAssetPoolCount   = 256;
	XAssetPools       = nullptr;
	Strings           = nullptr;
	StringPoolSize    = 0;
	Initialized       = false;
	StringLookupTable = nullptr;
	FileSystem        = nullptr;
	GameDirectory.clear();

	return true;
}

bool ps::CoDVGHandler::IsValid(const std::string& param)
{
	return strcmp(param.c_str(), "vg") == 0;
}

bool ps::CoDVGHandler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
{
	ps::log::Log(ps::LogType::Normal, "Attempting to load: %s using handler: %s...", ffName.c_str(), GetName().c_str());

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

	uint8_t ffHeader[152]{};
	uint8_t fpHeader[360]{};

	ffHandle.Read(&ffHeader[0], 0, sizeof(ffHeader));

	// We must check if we have a patch file, we'll need to take a different
	// route if we do during file reading to ensure we're patching with new data.
	if (fpHandle.IsValid())
	{
		fpHandle.Read(&fpHeader[0], 0, sizeof(fpHeader));

		// std::ofstream a0("header0.dat", std::ios::binary);
		// std::ofstream a1("header1.dat", std::ios::binary);

		// a0.write((const char*)&ffHeader[0], sizeof(ffHeader));
		// a1.write((const char*)&fpHeader[0], sizeof(fpHeader));

		if (std::memcmp(&ffHeader[0], &fpHeader[56], sizeof(ffHeader)) != 0)
		{
			ps::log::Log(ps::LogType::Error, "Current fast file header does not match the one within the patch file.");
			ps::log::Log(ps::LogType::Error, "This may indicate your install is corrupt, or the file provided is for content no longer available.");
			return false;
		}

		// Patch the fast file header
		std::memcpy(&ffHeader[0], &fpHeader[208], sizeof(ffHeader));
	}

	ps::CoDVGInternal::ResetPatchState(
		*(uint64_t*)&fpHeader[16],
		*(uint64_t*)&fpHeader[24],
		*(uint64_t*)&fpHeader[32],
		*(uint64_t*)&fpHeader[48]);

	// Verify XFile Version
	if (ps::CoDVGInternal::XFileVersions[*(uint8_t*)(ffHeader + 18)] != *(uint32_t*)(ffHeader + 12))
	{
		ps::log::Log(ps::LogType::Error, "Invalid XFile Version, got version: 0x%llx expected: 0x%llx", *(uint32_t*)(ffHeader + 12), ps::CoDVGInternal::XFileVersions[*(uint8_t*)(ffHeader + 18)]);
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

	ps::CoDVGInternal::FFDecompressor = std::make_unique<OodleDecompressorV1>(ffHandle, true);
	ps::CoDVGInternal::FPDecompressor = std::make_unique<OodleDecompressorV1>(fpHandle, true);

	ps::CoDVGInternal::InitializePatch();
	ps::CoDVGInternal::InitAssetAlignment();
	ps::CoDVGInternal::InitAssetAlignmentInternal();
	ps::CoDVGInternal::AssignFastFileMemoryPointers(&ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[0]);
	ps::CoDVGInternal::ZoneLoaderFlag[0] = 1;
	ps::CoDVGInternal::ParseFastFile(nullptr, ps::Parasyte::GetCurrentFastFile()->AssetList, ffName.c_str(), 0);
	ps::CoDVGInternal::FFDecompressor = nullptr;
	ps::CoDVGInternal::FPDecompressor = nullptr;

	ps::log::Log(ps::LogType::Normal, "Successfully loaded: %s", ffName.c_str());

	// If we have no parent, we are a root, and need to attempt to load the localized, etc.
	if (newFastFile->Parent == nullptr && !flags.HasFlag(FastFileFlags::NoChildren))
	{
		auto techsetsName = "techsets_" + ffName;
		auto wwName = "worldwide\\ww_" + ffName;

		// Attempt to load the techsets file, this usually contains
		// materials, shaders, technique sets, etc.
		if (DoesFastFileExists(techsetsName) && !IsFastFileLoaded(techsetsName))
			LoadFastFile(techsetsName, newFastFile, flags);
		// Attempt to load the ww file, same as locale, not as important
		// but we'll still load it, as it can contain data we need.
		if (DoesFastFileExists(wwName) && !IsFastFileLoaded(wwName))
			LoadFastFile(wwName, newFastFile, flags);

		// Check for locale prefix
		if (!RegionPrefix.empty())
		{
			auto localeName = RegionPrefix + ffName;

			// Now load the locale, not as important, as usually
			// only contains localized data.
			if (DoesFastFileExists(localeName) && !IsFastFileLoaded(localeName))
				LoadFastFile(localeName, newFastFile, flags);
		}
	}

	return true;
}

bool ps::CoDVGHandler::CleanUp()
{
	ps::CoDVGInternal::FFDecompressor = nullptr;
	ps::CoDVGInternal::FPDecompressor = nullptr;
	ps::CoDVGInternal::ResetPatchState(0, 0, 0, 0);
	return false;
}

const std::string ps::CoDVGHandler::GetShorthand()
{
	return "vg";
}

PS_CINIT(ps::GameHandler, ps::CoDVGHandler, ps::GameHandler::GetHandlers());
#endif