#include "pch.h"
#if _WIN64
#include "Parasyte.h"
#include "CoDMW5Handler.h"
#include "OodleDecompressorV2.h"

namespace ps::CoDMW5Internal
{
	// The fast file decompressor.
	std::unique_ptr<Decompressor> FFDecompressor;
	// The patch file decompressor.
	std::unique_ptr<Decompressor> FPDecompressor;

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
	char* (__cdecl* DecryptString)(void* a, size_t b, char* c, char* result);
	// Parses a fast file and all data within it.
	void* (__cdecl* ParseFastFile)(const void* a, const char* b, const char* c, bool d);
	// Assigns fast file memory pointers.
	void(__cdecl* AssignFastFileMemoryPointers)(void* blocks);
	// Adds an asset offset to the list.
	void(__cdecl* AddAssetOffset)(size_t* assetType);
	// Initializes Asset Alignment.
	void(__cdecl* InitAssetAlignmentInternal)();
	// Gets the xasset hash.
	uint64_t(__fastcall* GetXAssetHash)(uint32_t xassetType, void* xassetHeader);
	// Gets the xasset name.
	const char* (__fastcall* GetXAssetName)(uint32_t xassetType, void* xassetHeader);
	// Gets the xasset type name.
	const char* (__fastcall* GetXAssetTypeName)(uint32_t xassetType);
	// Gets the xasset header size.
	uint32_t(__fastcall* GetXAssetHeaderSize)(uint32_t xassetType);
	// Gets the xasset pool size.
	uint32_t(__fastcall* GetXAssetPoolSize)(uint32_t xassetType);
	// Checks if the xasset type has a name value.
	const char* (__fastcall* XAssetTypeHasName)(uint32_t xassetType);

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
	// A buffer for loading strings.
	std::unique_ptr<uint8_t[]> StrDecryptBuffer = nullptr;
	// The size of the above string buffer.
	constexpr size_t StrDecryptBufferSize = 65535;
	// Current string offset being allocated.
	uint32_t StrBufferOffset = 0;
	// XFile Versions
	// int* XFileVersions = nullptr;

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
	// Reads data from the Fast File. If patching is enabled, then data is consumed from the patch and fast file.
	void ReadXFile(char* pos, size_t size)
	{
		// We have a patch file included
		if (FPDecompressor->IsValid() && PatchFileState != nullptr && *(uint64_t*)(PatchFileState.get() + 528) > 0)
		{
			uint8_t* patch = PatchFileState.get();

			size_t current = *(uint32_t*)(patch + 472);
			size_t remaining = size;

			while (remaining > 0)
			{
				// We need to check if we need more data from the patch/fast file.
				if (*(uint32_t*)(patch + 456) == current)
				{
					*(uint64_t*)(patch + 472) = 0;

					if (!RequestPatchedData(
						PatchFileState.get(),
						PatchFileState.get() + 376,
						RequestFastFileData,
						RequestPatchFileData,
						RequestFinalFileData))
					{
						ps::log::Log(ps::LogType::Error, "Failed to patch fast file, error code: %lli", *(uint32_t*)(PatchFileState.get() + 380));
						throw std::exception("MW5_PatchFile_RequestData(...) failed");
					}
				}

				size_t size = (size_t)(*(uint32_t*)(patch + 456) - *(uint32_t*)(patch + 472));

				if (remaining < size)
					size = remaining;

				std::memcpy(pos, (void*)(*(uint64_t*)(patch + 472) + *(uint64_t*)(patch + 440)), size);
				*(uint32_t*)(patch + 472) += (uint32_t)size;
				pos += size;
				remaining -= size;

				current = *(uint32_t*)(patch + 472);
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
			if (*(void**)(PatchFileState.get() + 440) != nullptr)
				_aligned_free(*(void**)(PatchFileState.get() + 440));
			if (*(void**)(PatchFileState.get() + 408) != nullptr)
				_aligned_free(*(void**)(PatchFileState.get() + 408));
			if (*(void**)(PatchFileState.get() + 480) != nullptr)
				_aligned_free(*(void**)(PatchFileState.get() + 480));

			*(void**)(PatchFileState.get() + 440) = nullptr;
			*(void**)(PatchFileState.get() + 408) = nullptr;
			*(void**)(PatchFileState.get() + 480) = nullptr;
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

			*(void**)(PatchFileState.get() + 440) = _aligned_malloc(dataSize, 4096);
			*(void**)(PatchFileState.get() + 408) = _aligned_malloc(fastFileDataSize, 4096);
			*(void**)(PatchFileState.get() + 480) = _aligned_malloc(patchFileDataSize, 4096);
		}

		if(PatchFileState != nullptr)
			*(uint64_t*)(PatchFileState.get() + 528) = headerDecompressedSize;
	}
	// Allocates a unique string entry.
	void* AllocateUniqueString(char* a, char* str, int type)
	{
		char* decrypted = str;

		// Check if the string is actually encrypted.
		if ((*str & 0xC0) == 0x80)
			decrypted = DecryptString(StrDecryptBuffer.get(), StrDecryptBufferSize, str, nullptr);

		auto strLen = strlen(decrypted) + 1;
		auto id = XXHash64::hash(decrypted, strLen, 0);
		auto potentialEntry = ps::Parasyte::GetCurrentHandler()->StringLookupTable->find(id);

		if (potentialEntry != ps::Parasyte::GetCurrentHandler()->StringLookupTable->end())
		{
			StrBufferOffset = (uint32_t)potentialEntry->second;
			return &StrBufferOffset;
		}

		auto offset = ps::Parasyte::GetCurrentHandler()->StringPoolSize;
		std::memcpy(&ps::Parasyte::GetCurrentHandler()->Strings[offset], decrypted, strLen);

		ps::Parasyte::GetCurrentHandler()->StringPoolSize += strLen;
		ps::Parasyte::GetCurrentHandler()->StringLookupTable->operator[](id) = offset;

		StrBufferOffset = (uint32_t)offset;

		return &StrBufferOffset;
	}
	// Calls memset.
	void* Memset(void* ptr, size_t val, size_t size)
	{
		return std::memset(ptr, (int)val, size);
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
	// Links a generic xasset.
	void* LinkGenericXAsset(uint32_t assetType, char*** asset)
	{
		auto hash = GetXAssetHash(assetType, *asset);
		auto flag = hash >> 63;
		auto size = GetXAssetHeaderSize(assetType);
		auto temp = flag != 0;
		auto pool = &ps::Parasyte::GetCurrentHandler()->XAssetPools[assetType];

		// Some assets in MW2 have names, we need it for logging below
		// and to remove the appended comma to indicate a temp asset.
		const char* name = nullptr;

		if (XAssetTypeHasName(assetType))
		{
			name = GetXAssetName(assetType, *asset);

			if (name && temp && name[0] == ',')
			{
				std::memmove((void*)&name[0], &name[1], strlen(name));
			}
		}

		// Lazy allocate our pool to the default size from within the game.
		pool->Initialize(size, 256);

		// Remove status flag from the hash
		hash &= 0x7FFFFFFFFFFFFFFF;

		auto result = pool->FindXAssetEntry(hash, assetType);

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
				hash,
				assetType,
				size,
				ps::Parasyte::GetCurrentFastFile(),
				(uint8_t*)*asset,
				temp);
		}

		// If we're an image, we need to check if we want to allocate an image slot
		if (assetType == 19 && *(uint64_t*)(result->Header + 0x40) != 0 && result->ExtendedData == nullptr)
		{
			result->ExtendedDataSize = *(uint32_t*)(result->Header + 0x18);
			result->ExtendedData = std::make_unique<uint8_t[]>(result->ExtendedDataSize);
			result->ExtendedDataPtrOffset = 0x40;
			std::memcpy(result->ExtendedData.get(), *(uint8_t**)(result->Header + 0x40), result->ExtendedDataSize);
			*(uint64_t*)(result->Header + 0x40) = (uint64_t)result->ExtendedData.get();
		}
// #if PRIVATE_GRAM_GRAM
		if (assetType == 60 && DecryptString != nullptr)
		{
			char* str = *(char**)(result->Header + 8);
			if ((*str & 0xC0) == 0x80)
			{
				str = DecryptString(ps::CoDMW5Internal::StrDecryptBuffer.get(), ps::CoDMW5Internal::StrDecryptBufferSize, str, nullptr);
			}
			memcpy(*(char**)(result->Header + 8), str, strlen(str) + 1);
		}
// #else
// 		switch (assetType)
// 		{
// 		// Kill localized
// 		case 60:
// 		{
// 			char* str = *(char**)(result->Header + 8);
// 			std::memset(str, 0, strlen(str));
// 			*(uint64_t*)result->Header = 0;
// 			break;
// 		}
// 		// Kill Lua files
// 		case 82:
// 		{
// 			*(uint64_t*)result->Header = 0;
// 			auto sizeOfRawFile = *(uint32_t*)(result->Header + 8);
// 			auto rawFileData = *(char**)(result->Header + 16);
// 			if (rawFileData != nullptr) std::memset(rawFileData, 0, sizeOfRawFile);
// 			break;
// 		}
// 		// Kill raw files
// 		case 70:
// 		{
// 			*(uint64_t*)result->Header = 0;
// 			auto sizeOfRawFile = *(uint32_t*)(result->Header + 12);
// 			auto rawFileData = *(char**)(result->Header + 24);
// 			if (sizeOfRawFile == 0)
// 				sizeOfRawFile = *(uint32_t*)(result->Header + 16) + 1;
// 			if (rawFileData != nullptr) std::memset(rawFileData, 0, sizeOfRawFile);
// 			break;
// 		}
// 		}
// #endif

		size_t toPop[2]{ assetType, (size_t)*asset };
		AddAssetOffset(toPop);

		// Loggary for Stiggary
		ps::log::Log(ps::LogType::Verbose, "Linked: 0x%llx (Name: %s) Type: 0x%llx (%s) @ 0x%llx", hash, name, (uint64_t)assetType, GetXAssetTypeName(assetType), (uint64_t)result->Header);

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
	// yer boio
	// void __fastcall sub_7FF6E22E53A0(__int64 a1)
	// {
	// 	int v2; // eax
	// 	int v3; // ebp
	// 	int v4; // edi
	// 	__int64 v5; // rsi
	// 	__int64* v6; // rcx
	// 	__int64 v7; // rax
	// 	__int64 v8; // rdx
	//
	// 	*(uint32_t*)(a1 + 36) &= 0xFFFCFFFF;
	// 	v2 = *(uint8_t*)(a1 + 18);
	// 	v3 = 0;
	// 	v4 = 0;
	// 	if ((uint8_t)v2)
	// 	{
	// 		v5 = 0i64;
	// 		do
	// 		{
	// 			v6 = (__int64*)(v5 + *(uint64_t*)(a1 + 256));
	// 			v7 = *v6;
	// 			if (*v6)
	// 			{
	// 				v8 = *(uint64_t*)(v7 + 8);
	// 				v6[1] = v8;
	// 			}
	// 			else
	// 			{
	// 				v6[1] = 0i64;
	// 			}
	// 			v2 = *(unsigned __int8*)(a1 + 18);
	// 			++v4;
	// 			v5 += 72i64;
	// 		} while (v4 < v2);
	// 	}
	// 	if (v3 == (unsigned __int8)v2)
	// 		*(uint32_t*)(a1 + 36) |= 0x20000u;
	// }
}

const std::string ps::CoDMW5Handler::GetName()
{
	return "Call of Duty: Modern Warfare 2 (2022)";
}

bool ps::CoDMW5Handler::Initialize(const std::string& gameDirectory)
{
	Configs.clear();
	GameDirectory = gameDirectory;

	if (!LoadConfigs("CoDMW5Handler"))
	{
		return false;
	}

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

	// PS_SETGAMEVAR(ps::CoDMW5Internal::XFileVersions);
	PS_SETGAMEVAR(ps::CoDMW5Internal::GetXAssetHeaderSize);
	PS_SETGAMEVAR(ps::CoDMW5Internal::GetXAssetPoolSize);
	PS_SETGAMEVAR(ps::CoDMW5Internal::RequestPatchedData);
	PS_SETGAMEVAR(ps::CoDMW5Internal::RequestFastFileData);
	PS_SETGAMEVAR(ps::CoDMW5Internal::RequestPatchFileData);
	PS_SETGAMEVAR(ps::CoDMW5Internal::InitializePatch);
	PS_SETGAMEVAR(ps::CoDMW5Internal::RequestFastFileData);
	PS_SETGAMEVAR(ps::CoDMW5Internal::RequestFinalFileData);
	PS_SETGAMEVAR(ps::CoDMW5Internal::AssignFastFileMemoryPointers);
	PS_SETGAMEVAR(ps::CoDMW5Internal::InitAssetAlignmentInternal);
	PS_SETGAMEVAR(ps::CoDMW5Internal::AddAssetOffset);
	PS_SETGAMEVAR(ps::CoDMW5Internal::XAssetOffsetList);
	PS_SETGAMEVAR(ps::CoDMW5Internal::ZoneLoaderFlag);
	PS_SETGAMEVAR(ps::CoDMW5Internal::ParseFastFile);
	PS_SETGAMEVAR(ps::CoDMW5Internal::GetXAssetHash);
	PS_SETGAMEVAR(ps::CoDMW5Internal::GetXAssetName);
	PS_SETGAMEVAR(ps::CoDMW5Internal::XAssetTypeHasName);
	PS_SETGAMEVAR(ps::CoDMW5Internal::GetXAssetHeaderSize);
	PS_SETGAMEVAR(ps::CoDMW5Internal::GetXAssetTypeName);
	PS_SETGAMEVAR(ps::CoDMW5Internal::DecryptString);

	PS_DETGAMEVAR(ps::CoDMW5Internal::ReadXFile);
	PS_DETGAMEVAR(ps::CoDMW5Internal::AllocateUniqueString);
	PS_DETGAMEVAR(ps::CoDMW5Internal::LinkGenericXAsset);
	PS_DETGAMEVAR(ps::CoDMW5Internal::LinkGenericXAssetEx);
	PS_DETGAMEVAR(ps::CoDMW5Internal::ReadPatchFile);
	PS_DETGAMEVAR(ps::CoDMW5Internal::ReadFastFile);
	PS_DETGAMEVAR(ps::CoDMW5Internal::Memset);

	XAssetPoolCount   = 256;
	XAssetPools       = std::make_unique<XAssetPool[]>(XAssetPoolCount);
	Strings           = std::make_unique<char[]>(0x2000000);
	StringPoolSize    = 0;
	Initialized       = true;
	StringLookupTable = std::make_unique<std::map<uint64_t, size_t>>();

	// Game specific buffers.
	ps::CoDMW5Internal::XAssetAlignmentBuffer = std::make_unique<uint8_t[]>((size_t)65535 * 32);
	ps::CoDMW5Internal::StrDecryptBuffer = std::make_unique<uint8_t[]>(ps::CoDMW5Internal::StrDecryptBufferSize);

	Module.SaveCache(CurrentConfig->CacheName);
	LoadAliases(CurrentConfig->AliasesName);

	return true;
}

bool ps::CoDMW5Handler::Deinitialize()
{
	Module.Free();
	XAssetPoolCount       = 256;
	XAssetPools           = nullptr;
	Strings               = nullptr;
	StringPoolSize        = 0;
	Initialized           = false;
	StringLookupTable     = nullptr;
	FileSystem            = nullptr;
	GameDirectory.clear();

	// Clear game specific buffers
	ps::CoDMW5Internal::ResetPatchState(0, 0, 0, 0);
	ps::CoDMW5Internal::PatchFileState = nullptr;
	ps::CoDMW5Internal::XAssetAlignmentBuffer = nullptr;
	ps::CoDMW5Internal::StrDecryptBuffer = nullptr;

	ps::oodle::Clear();

	return true;
}

bool ps::CoDMW5Handler::IsValid(const std::string& param)
{
	return strcmp(param.c_str(), "mw5") == 0;
}

bool ps::CoDMW5Handler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
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

	uint8_t ffHeader[216]{};
	uint8_t fpHeader[488]{};

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
		std::memcpy(&ffHeader[0], &fpHeader[272], sizeof(ffHeader));
	}

	ps::CoDMW5Internal::ResetPatchState(
		*(uint64_t*)&fpHeader[16],
		*(uint64_t*)&fpHeader[24],
		*(uint64_t*)&fpHeader[32],
		*(uint64_t*)&fpHeader[48]);

	uint64_t* bufferSizes = (uint64_t*)&ffHeader[64];

	for (size_t i = 0; i < 17; i++)
	{
		if (bufferSizes[i] > 0)
		{
			ps::log::Log(ps::LogType::Verbose, "Allocating block: %llu of size: 0x%llx", i, bufferSizes[i]);
			ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[i].Initialize(bufferSizes[i], 4096);
		}
	}

	ps::CoDMW5Internal::FFDecompressor = std::make_unique<OodleDecompressorV2>(ffHandle, true);
	ps::CoDMW5Internal::FPDecompressor = std::make_unique<OodleDecompressorV2>(fpHandle, true);

	ps::CoDMW5Internal::InitAssetAlignment();
	ps::CoDMW5Internal::InitAssetAlignmentInternal();
	ps::CoDMW5Internal::AssignFastFileMemoryPointers(&ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[0]);
	ps::CoDMW5Internal::ZoneLoaderFlag[0] = 1;
	ps::CoDMW5Internal::ParseFastFile(nullptr, ps::Parasyte::GetCurrentFastFile()->AssetList, ffName.c_str(), 0);

	ps::CoDMW5Internal::FFDecompressor = nullptr;
	ps::CoDMW5Internal::FPDecompressor = nullptr;

	//// We must fix up any XModel surfs, as we may have overrode previous
	//// temporary entries, etc.
	//ps::Parasyte::GetCurrentHandler()->XAssetPools[9].EnumerateEntries([](ps::XAsset* asset)
	//{
	//	ps::CoDMW5Internal::sub_7FF6E22E53A0((__int64)asset->Header);
	//});

	ps::log::Log(ps::LogType::Normal, "Successfully loaded: %s", ffName.c_str());

	// If we have no parent, we are a root, and need to attempt to load the localized, etc.
	if (newFastFile->Parent == nullptr && !flags.HasFlag(FastFileFlags::NoChildren))
	{
		auto techsetsName = "techsets_" + ffName;
		auto wwName = "ww_" + ffName;

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

bool ps::CoDMW5Handler::CleanUp()
{
	ps::CoDMW5Internal::FFDecompressor = nullptr;
	ps::CoDMW5Internal::FPDecompressor = nullptr;
	ps::CoDMW5Internal::ResetPatchState(0, 0, 0, 0);
	return false;
}

std::string ps::CoDMW5Handler::GetFileName(const std::string& name)
{
	return (CurrentConfig->FilesDirectory.empty()) ? (name) : (CurrentConfig->FilesDirectory + "/" + name);
}

const std::string ps::CoDMW5Handler::GetShorthand()
{
	return "mw5";
}

PS_CINIT(ps::GameHandler, ps::CoDMW5Handler, ps::GameHandler::GetHandlers());
#endif