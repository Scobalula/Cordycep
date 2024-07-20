#include "pch.h"

#if _WIN64
#include "Parasyte.h"
#include "CoDGhostsHandler.h"
#include "lz4.h"
#include "ZLIBDecompressorV1.h"

// #include <xmmintrin.h>

namespace ps::CoDGhostsInternal
{
	// The fast file decompressor.
	// TODO:
	std::unique_ptr<ps::ZLIBDecompressorV1> FFDecompressor;

	// Sound Files
	std::array<HANDLE, 512> SoundFiles;
	// Localized Sound Files
	std::array<HANDLE, 512> LocalizedSoundFiles;

	// void (__fastcall* DB_LoadXFile)(XZoneMemory *zoneMem, DBFile *file);
	void (__cdecl* DB_LoadXFile)(const void* zoneMem, const void* file);

	const char* (__fastcall* DB_GetXAssetName)(void* xassetHeader);

	const char* (__fastcall* DB_SetXAssetName)(void* xassetHeader, const char* name);

	size_t (__fastcall* DB_GetXAssetTypeSizeInternal)(uint32_t xassetType);

	int64_t (__fastcall* DB_XModelSurfsFixup)(uint8_t* a1);

	// Blank Graphics Dvar Buffer
	char BlankGraphicsDvarBuffer[512]{};
	// Graphics Related Dvar
	char** G_GraphicsDvar = nullptr;

	// Images Count
	uint32_t* G_ImageCount = nullptr;
	// Current Images Index
	uint32_t G_ImageIndex = 0;
	// Images Buffer
	struct G_XImageData // XStreamFile
	{
		uint16_t IsLocalized;
		uint16_t PackageIndex;
		uint32_t Checksum;
		uint64_t Offset;
		uint64_t OffsetEnd;
	};
	G_XImageData* G_ImagesBuffer = nullptr;

	// Zone Memory Block
	struct G_XBlock
	{
		void* data;
		size_t size;
	};

	// Zone Memory
	struct G_XZoneMemory
	{
		G_XBlock blocks[7];
	};

	char** g_assetNames = nullptr;

	enum XAssetType : int32_t
	{
		ASSET_TYPE_XMODEL = 4, // 0x4
		ASSET_TYPE_IMAGE = 13, // 0xD
		ASSET_TYPE_LOADED_SOUND = 18, // 0x12
	};

	enum XAssetTypeSize : size_t
	{
		ASSET_TYPE_SIZE_IMAGE = 104,
	};

	// Resets all data.
	void G_DB_Reset()
	{
		G_ImageIndex = 0;
	}

	// Requests sound file data for loaded audio.
	void DB_RequestSoundFileData(void* ptr, uint16_t index, size_t fileOffset, size_t size, const char* name, bool isLocalized)
	{
		if (!ptr)
			return;
		if (fileOffset == 0 || size == 0)
			return;

		HANDLE handle = isLocalized ? LocalizedSoundFiles[index] : SoundFiles[index];

		if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
		{
			auto soundFilePath = "soundfile" + std::to_string(index) + ".pak";

			if (isLocalized)
			{
				if (!ps::Parasyte::GetCurrentHandler()->RegionPrefix.empty())
				{
					soundFilePath = ps::Parasyte::GetCurrentHandler()->RegionPrefix + soundFilePath;
				}
				else
				{
					ps::log::Log(ps::LogType::Verbose, "WARNING: Localized audio requested by: %s but no localization prefix is set.", name);
					return;
				}
			}

			soundFilePath = ps::Parasyte::GetCurrentHandler()->GameDirectory + "\\" + soundFilePath;

			handle = CreateFileA(
				soundFilePath.c_str(),
				FILE_READ_DATA | FILE_READ_ATTRIBUTES,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
			   nullptr,
				OPEN_EXISTING,
				0,
			nullptr);

			if(isLocalized)
				LocalizedSoundFiles[index] = handle;
			else
				SoundFiles[index] = handle;
		}

		if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER Input{};
			Input.QuadPart = fileOffset;

			if (SetFilePointerEx(handle, Input, nullptr, FILE_BEGIN))
			{
				if (!ReadFile(handle, ptr, (DWORD)size, nullptr, nullptr))
				{
					ps::log::Log(ps::LogType::Verbose, "WARNING: Failed to load sound data for sound: %s.", name);
				}
			}
		}
		else
		{
			ps::log::Log(ps::LogType::Verbose, "WARNING: Failed to open sound file for sound: %s.", name);
		}
	}

	// Gets the xasset type name.
	const char* GetXAssetTypeName(uint32_t xassetType)
	{
		if (!g_assetNames)
			return "null";

		const auto typeName = g_assetNames[xassetType];

		return typeName ? typeName : "null";
	}

	// Allocates a unique string entry.
	size_t SL_GetString(const char* str, unsigned int user)
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

	void DB_ReadXFile(void* ptr, size_t size)
	{
		const auto resultSize = FFDecompressor->Read(ptr, size, 0);
		if (!resultSize)
			__debugbreak();
	}

	// Reads uncompressed data from the file while accounting for hash blocks.
	void DB_ReadXFileUncompressed(void* pos, size_t size)
	{
		FFDecompressor->ReadUncompressed((uint8_t*)pos, size);
		// (std::unique_ptr<ps::ZLIBDecompressorV1>)(FFDecompressor) // TODO: how
	}

	void DB_AllocXZoneMemory(uint64_t* blockSize, const char* filename, G_XZoneMemory* zoneMem)
	{
		for (int i = 0; i < 7; i++)
		{
			ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[i].Initialize(blockSize[i], 0x1000);

			zoneMem->blocks[i].size = ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[i].MemorySize;
			zoneMem->blocks[i].data = ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[i].Memory;
		}
	}

	void DB_InflateInit(int fileIsSecure)
	{
		// MWR_FileIsSecure = fileIsSecure;
	}

	void DB_SetFileLoadCompressor(int compression)
	{
		// FileCompression = compression;
	}

	typedef uint32_t G_XAssetType;

	struct G_XAssetHeader
	{
		void* Header;
	};

	struct G_XAsset
	{
		G_XAssetType type;
		G_XAssetHeader header;
	};

	struct G_XAssetEntry
	{
		G_XAsset asset;
		char zoneIndex;
		volatile char inuseMask;
		unsigned int nextHash;
		unsigned int nextOverride;
		unsigned int nextPoolEntry;
	};

	G_XAssetEntry G_DB_LinkedXAsset{};

	size_t DB_GetXAssetTypeSize(uint32_t xassetType)
	{
		auto result = DB_GetXAssetTypeSizeInternal(xassetType);

		// Add on extra for asset specific data that
		// the game would usually store in a table
		switch (xassetType)
		{
			case XAssetType::ASSET_TYPE_IMAGE:
			{
				result += 128; // more than streamed mips struct size (4*24)
				break;
			}
		}

		return result;
	}

	G_XAssetEntry* DB_LinkXAssetEntry1(G_XAssetType xassetType, G_XAssetHeader* asset)
	{
		G_XAsset xasset
		{
			xassetType,
			*asset
		};

		auto name = DB_GetXAssetName(&xasset);
		auto pool = &ps::Parasyte::GetCurrentHandler()->XAssetPools[xassetType];
		auto temp = name[0] == ',';
		auto size = DB_GetXAssetTypeSize(xassetType);

		// Ensure pool is initialized
		pool->Initialize(size, 16384);

		if (temp)
		{
			DB_SetXAssetName(&xasset, name + 1);
			name = DB_GetXAssetName(&xasset);
		}

		// Our result
		auto result = pool->FindXAssetEntry(name, strlen(name), xassetType);

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
					(uint8_t*)asset->Header,
					temp);
			}
			else
			{
				result->Override(
					ps::Parasyte::GetCurrentFastFile(),
					(uint8_t*)asset->Header,
					temp);
			}
		}

		// If we got to this point, we haven't found the entry, so let's create it
		if (result == nullptr)
		{
			result = pool->CreateEntry(
				name,
				strlen(name),
				xassetType,
				(uint32_t)size,
				ps::Parasyte::GetCurrentFastFile(),
				(uint8_t*)asset->Header,
				temp);
		}

		// Loggary for Stiggary
		ps::log::Log(ps::LogType::Verbose, "Linked XAsset: \"%s\", Type: %i(%s), @ %llx", name, xassetType, GetXAssetTypeName(xassetType), (uint64_t)result->Header);

		G_DB_LinkedXAsset.asset.type = xassetType;
		G_DB_LinkedXAsset.asset.header.Header = result->Header;

		auto header = result->Header;

		if (!temp)
		{
			switch (xassetType)
			{
			case XAssetType::ASSET_TYPE_IMAGE:
			{
				// Check are we a streamed image, if we are, we need to grab the streaming
				// info that was loaded from the fast file's header. The game resolves this 
				// later so we must do it now.
				if (*(uint8_t*)(header + 53) != 0) // streamed
				{
					// No image table but we have images, so attempt locate from another image?
					// Haven't the slightest clue how the game accounts for this rn but we'll do it
					// like Harry's marriage.
					if (*G_ImageCount == 0)
					{
						// Gets the first non-temp child that is owned by a root file.
						auto xasset = result->FindChildOfFastFile(ps::Parasyte::GetCurrentFastFile()->Parent);

						if (xasset != nullptr)
						{
							std::memcpy(
								header + XAssetTypeSize::ASSET_TYPE_SIZE_IMAGE + sizeof(G_XImageData),
								xasset->Header + XAssetTypeSize::ASSET_TYPE_SIZE_IMAGE + sizeof(G_XImageData),
								sizeof(G_XImageData) * 4);
						}
					}
					else
					{
						for (size_t i = 0; i < 4; i++)
						{
							// Should never happen on a valid Fast File
							if (G_ImageIndex >= *G_ImageCount)
								throw std::exception("Invalid image index, corrupt PAK table in fast file.");
	
							auto image = G_ImagesBuffer[G_ImageIndex++];
							// Copy to end of the header so that tools can read directly from it
							std::memcpy(
								header + XAssetTypeSize::ASSET_TYPE_SIZE_IMAGE + sizeof(G_XImageData) * i,
								&image,
								sizeof(G_XImageData));
							ps::log::Log(ps::LogType::Verbose, "Mip Map Pkg: %llu @ 0x%llx", (uint64_t)image.PackageIndex, image.Offset);
						}
					}
				}
				else if (*(uint64_t*)(header + 56) != 0) // pixelData ptr
				{
					result->ExtendedDataSize = *(uint32_t*)(header + 40);
					result->ExtendedData = std::make_unique<uint8_t[]>(result->ExtendedDataSize);
					result->ExtendedDataPtrOffset = 56;
					std::memcpy(result->ExtendedData.get(), *(void**)(header + 56), result->ExtendedDataSize);
					*(void**)(header + 56) = result->ExtendedData.get();
				}
				break;
			}
			case XAssetType::ASSET_TYPE_LOADED_SOUND:
			{
				DB_RequestSoundFileData(
					*(void**)(header + 32),
					*(uint16_t*)(header + 10),
					*(uint64_t*)(header + 16),
					*(uint32_t*)(header + 24),
					name,
					*(uint8_t*)(header + 8));
				break;
			}
			}
		}

		return &G_DB_LinkedXAsset;
	}

	void* DB_FindXAssetHeader(G_XAssetType xassetType, const char* name, int allowCreateDefault)
	{
		auto pool = &ps::Parasyte::GetCurrentHandler()->XAssetPools[xassetType];
		auto result = pool->FindXAssetEntry(name, strlen(name), xassetType);

		if (result != nullptr)
		{
			return result->Header;
		}
		else if (allowCreateDefault == 1)
		{
			result = pool->CreateEntry(
				name,
				strlen(name),
				xassetType,
				(uint32_t)DB_GetXAssetTypeSize(xassetType),
				ps::Parasyte::GetCurrentFastFile(),
				nullptr,
				true);

			G_XAsset asset
			{
				xassetType,
				result->Header
			};
			DB_SetXAssetName(&asset, name);

			return result->Header;
		}

		return nullptr;
	}

	// TODO: Not finished
	/*int64_t __fastcall DB_XModelSurfsFixup(uint8_t* a1)
	{
		__int16 v1; // r9
		int v2; // r10d
		uint64_t* v3; // rdx
		__int64 v4; // r8
		__int16 v5; // ax
		__int64 result; // rax

		a1[499] &= ~2u;
		v1 = 0;
		v2 = 0;
		if ( (char)a1[497] > 0 )
		{
			v3 = (uint64_t*)a1 + 160;
			do
			{
				v4 = *(v3 - 5);
				*((__m128 *)v3 - 8) = *(__m128 *)(v4 + 20);
				*((__m128 *)v3 - 7) = *(__m128 *)(v4 + 24);
				*((__m128 *)v3 - 6) = *(__m128 *)(v4 + 28);
				*((__m128 *)v3 - 5) = *(__m128 *)(v4 + 32);
				*((__m128 *)v3 - 4) = *(__m128 *)(v4 + 36);
				*((__m128 *)v3 - 3) = *(__m128 *)(v4 + 40);
				*((__m128 *)v3 - 2) = *(__m128 *)(v4 + 44);
				*((__m128 *)v3 - 1) = *(__m128 *)(v4 + 48);
				*v3 = *(uint64_t *)(v4 + 8);
				v5 = *(uint16_t *)(v4 + 16);
				*((uint16_t *)v3 - 21) = v1;
				*((uint16_t *)v3 - 22) = v5;
				// if ( *(_QWORD *)(v4 + 8) == /*qword_141F8AA78#1#g_defaultSurfs )
				// 	a1[499] |= 2u;
				++v2;
				v3 += 8;
				v1 += *(uint16_t *)(v4 + 16);
				result = (unsigned int)(char)a1[497];
			}
			while ( v2 < (int)result );
		}
		a1[10] = v1;
		return result;
	}*/
}

const std::string ps::CoDGhostsHandler::GetShorthand()
{
	return "ghosts";
}

const std::string ps::CoDGhostsHandler::GetName()
{
	return "Call of Duty: Ghosts";
}

bool ps::CoDGhostsHandler::Initialize(const std::string& gameDirectory)
{
	Configs.clear();
	GameDirectory = gameDirectory;

	if (!LoadConfigs("CoDGhostsHandler"))
	{
		return false;
	}

	SetConfig();
	CopyDependencies();
	OpenGameDirectory(GameDirectory);
	OpenGameModule(CurrentConfig->ModuleName);

	ps::log::Log(ps::LogType::Verbose, "Module base address: 0x%llx", Module.Handle);

	Module.LoadCache(CurrentConfig->CacheName);

	ResolvePatterns();

	Pattern pattern;
    bool success = true;

	// Null and Patch the Return func of DB_LoadXFile()
    // Patch obfuscated call, restore original values and return back.
	pattern.Update("E9 ?? ?? ?? ?? 48 03 64 24 08 5B 59 58 5A E9 E6 25 0C 0A");
	if (!Module.PatchBytes(
		pattern,
		0,
		"Ghosts::NullDB_LoadXFileReturnFunc",
		(PBYTE)"\x48\x81\xC4\xB0\x00\x00\x00\x90\x90\x90\x41\x5E\x5F\x5D\x90\x90\x90\x90\xC3",
		20,
		true,
		false)) success = false;

    if (!success)
        return false;

	PS_SETGAMEVAR(ps::CoDGhostsInternal::DB_LoadXFile);
	PS_SETGAMEVAR(ps::CoDGhostsInternal::DB_GetXAssetName);
	PS_SETGAMEVAR(ps::CoDGhostsInternal::DB_SetXAssetName);
	PS_SETGAMEVAR(ps::CoDGhostsInternal::DB_XModelSurfsFixup);

	PS_SETGAMEVAR(ps::CoDGhostsInternal::g_assetNames);
	PS_SETGAMEVAR(ps::CoDGhostsInternal::G_GraphicsDvar);
	PS_SETGAMEVAR(ps::CoDGhostsInternal::G_ImageCount);
	PS_SETGAMEVAR(ps::CoDGhostsInternal::G_ImagesBuffer);

	PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_ReadXFile);
	PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_ReadXFileUncompressed);
	PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_AllocXZoneMemory);
	PS_DETGAMEVAR(ps::CoDGhostsInternal::SL_GetString);
	PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_LinkXAssetEntry1);
	PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_FindXAssetHeader);
	PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_InflateInit);
	PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_SetFileLoadCompressor);
	// PS_DETGAMEVAR(ps::CoDGhostsInternal::DB_XModelSurfsFixup); // TODO: Check if we need

	PS_INTGAMEVAR(ps::CoDGhostsInternal::DB_GetXAssetTypeSize, ps::CoDGhostsInternal::DB_GetXAssetTypeSizeInternal);

	XAssetPoolCount   = 256;
	XAssetPools       = std::make_unique<XAssetPool[]>(XAssetPoolCount);
	Strings           = std::make_unique<char[]>(0x2000000);
	StringPoolSize    = 0;
	Initialized       = true;
	StringLookupTable = std::make_unique<std::map<uint64_t, size_t>>();

	// Game specific buffers.
	*CoDGhostsInternal::G_GraphicsDvar = CoDGhostsInternal::BlankGraphicsDvarBuffer;

	Module.SaveCache(CurrentConfig->CacheName);
	LoadAliases(CurrentConfig->AliasesName);

	return true;
}

bool ps::CoDGhostsHandler::Deinitialize()
{
	CoDGhostsInternal::G_DB_Reset();

	Module.Free();
	XAssetPoolCount        = 256;
	XAssetPools            = nullptr;
	Strings                = nullptr;
	StringPoolSize         = 0;
	Initialized            = false;
	StringLookupTable      = nullptr;
	FileSystem             = nullptr;
	GameDirectory.clear();

	// Clear out open handles to reference files.
	for (auto& SoundFile : CoDGhostsInternal::SoundFiles)
	{
		if (SoundFile != nullptr && SoundFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(SoundFile);
			SoundFile = nullptr;
		}
	}

	for (auto& LocalizedSoundFile : CoDGhostsInternal::LocalizedSoundFiles)
	{
		if (LocalizedSoundFile != nullptr && LocalizedSoundFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(LocalizedSoundFile);
			LocalizedSoundFile = nullptr;
		}
	}

	return true;
}

bool ps::CoDGhostsHandler::IsValid(const std::string& param)
{
	return param == GetShorthand();
}

bool ps::CoDGhostsHandler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
{
	ps::log::Log(ps::LogType::Normal, "Attempting to load: %s using handler: %s...", ffName.c_str(), GetName().c_str());

	auto newFastFile = CreateUniqueFastFile(ffName);

	newFastFile->Parent = parent;
	newFastFile->Flags = flags;

	// Set current ff for loading purposes.
	ps::Parasyte::PushTelemtry("LastFastFileName", ffName);
	ps::Parasyte::SetCurrentFastFile(newFastFile);

	ps::FileHandle ffHandle(FileSystem->OpenFile(GetFileName(ffName + ".ff"), "r"), FileSystem.get());

	if (!ffHandle.IsValid())
	{
		ps::log::Log(ps::LogType::Error, "The provided fast file could not be found in the game's file system.");
		ps::log::Log(ps::LogType::Error, "Make sure any updates are finished and check for any content packages.");

		throw std::exception("Could not open the fast file from the game's file system.");
	}

	auto magic = ffHandle.Read<uint64_t>();
	auto version = ffHandle.Read<uint32_t>();

	if (magic != 0x3030313066665749 && magic != 0x3030317566665749)
		throw std::exception("Invalid fast file magic number.");
	if (version != 0x235)
		throw std::exception("Invalid fast file version.");

	/*
    auto flag0 = ffHandle.Read<uint8_t>(); // compress
	auto flag1 = ffHandle.Read<uint8_t>(); // compressType
	auto flag2 = ffHandle.Read<uint8_t>(); // sizeofPointer
	auto flag3 = ffHandle.Read<uint8_t>(); // sizeofLong
	auto data0 = ffHandle.Read<uint32_t>(); // fileTimeHigh
	auto data1 = ffHandle.Read<uint32_t>(); // fileTimeLow

	auto imageCount = ffHandle.Read<uint32_t>();
	auto imageData = ffHandle.ReadArray<uint8_t>((size_t)imageCount * 24);

	auto totalSize = ffHandle.Read<uint64_t>();
	auto authSize = ffHandle.Read<uint64_t>();
    */

	ffHandle.Seek(0, SEEK_SET);

	const bool isSecure = magic == 0x3030313066665749;
	ps::CoDGhostsInternal::FFDecompressor = std::make_unique<ZLIBDecompressorV1>(ffHandle, isSecure);

	bool success = true;

	CoDGhostsInternal::G_DB_Reset();

	CoDGhostsInternal::G_XZoneMemory memory{};
	CoDGhostsInternal::DB_LoadXFile(&memory, nullptr);

	CleanUp();

	ps::log::Log(ps::LogType::Normal, "Successfully loaded: %s", ffName.c_str());

	// We must fix up any XModel surfs, as we may have overrode previous
	// temporary entries, etc.
	ps::Parasyte::GetCurrentHandler()->XAssetPools[CoDGhostsInternal::XAssetType::ASSET_TYPE_XMODEL].EnumerateEntries([](ps::XAsset* asset)
	{
		CoDGhostsInternal::DB_XModelSurfsFixup(asset->Header);
	});

	// If we have no parent, we are a root, and need to attempt to load the localized, etc.
	if (success && newFastFile->Parent == nullptr)
	{
		auto techsetsName = "techsets_" + ffName;
		auto patchName = "patch_" + ffName;
		auto loadName = ffName + "_load";
		auto pathName = ffName + "_path";

		// Attempt to load the techsets file, this usually contains
		// materials, shaders, technique sets, etc.
		if (DoesFastFileExists(techsetsName) && !IsFastFileLoaded(techsetsName))
			LoadFastFile(techsetsName, newFastFile, flags);
		if (DoesFastFileExists(patchName) && !IsFastFileLoaded(patchName))
			LoadFastFile(patchName, newFastFile, flags);
		if (DoesFastFileExists(loadName) && !IsFastFileLoaded(loadName))
			LoadFastFile(loadName, newFastFile, flags);
		if (DoesFastFileExists(pathName) && !IsFastFileLoaded(pathName))
			LoadFastFile(pathName, newFastFile, flags);

		// Check for locale prefix
		if (!RegionPrefix.empty())
		{
			auto localeName = RegionPrefix + ffName;
			auto localePatchName = RegionPrefix + "patch_" + ffName;
			auto localeLoadName = RegionPrefix + ffName + "_load";
			auto localePathName = RegionPrefix + ffName + "_path";

			// Now load the locale, not as important, as usually
			// only contains localized data.
			if (DoesFastFileExists(localeName) && !IsFastFileLoaded(localeName))
				LoadFastFile(localeName, newFastFile, flags);
			if (DoesFastFileExists(localePatchName) && !IsFastFileLoaded(localePatchName))
				LoadFastFile(localePatchName, newFastFile, flags);
			if (DoesFastFileExists(localeLoadName) && !IsFastFileLoaded(localeLoadName))
				LoadFastFile(localeLoadName, newFastFile, flags);
			if (DoesFastFileExists(localePathName) && !IsFastFileLoaded(localePathName))
				LoadFastFile(localePathName, newFastFile, flags);
		}
	}

	return true;
}

bool ps::CoDGhostsHandler::CleanUp()
{
	ps::CoDGhostsInternal::G_DB_Reset();
	ps::CoDGhostsInternal::FFDecompressor = nullptr;
	return true;
}

PS_CINIT(ps::GameHandler, ps::CoDGhostsHandler, ps::GameHandler::GetHandlers());
#endif