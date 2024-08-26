#include "pch.h"
#if _WIN64
#include "Parasyte.h"
#include "CoDMW6Handler.h"
#include "OodleDecompressorV3.h"

#include <nlohmann/json.hpp>
using json = nlohmann::ordered_json;

#include <unordered_set>

namespace ps::CoDMW6Internal
{
	// The fast file decompressor.
	std::unique_ptr<Decompressor> FFDecompressor;
	// The patch file decompressor.
	std::unique_ptr<Decompressor> FPDecompressor;

	// Resolves the stream position from within the game.
	__int64(__cdecl* ResolveStreamPositionOriginal)(__int64* a1) = nullptr;
	// Initializes the patch function info.
	uint64_t(__fastcall* InitializePatch)();
	// Loads from the data stream.
	bool (__fastcall* LoadStream)(uint8_t* a1, uint64_t* a2, uint64_t* a3);
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
	// LoadStream function pointers
	uint64_t** LoadStreamFuncPointers = nullptr;
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

	// Resolves the stream position, running bounds checks.
	__int64 __fastcall ResolveStreamPosition(__int64* a1)
	{
		size_t zoneIndex = (*a1 >> 32) & 0xF;
		size_t zoneOffset = (size_t)((uint32_t)*a1 - 1);

		// A very basic check to see if this offset is
		// outside the bounds of the zone buffers.
		// Seems to work pretty well to avoid crashing for invalid files
		// from previous updates.
		// TODO:
		// if (zoneIndex >= 11)
		// 	throw std::exception("This file is from a previous update, skipping.");
		if (zoneOffset >= ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[zoneIndex].MemorySize)
			throw std::exception("This file is from a previous update, skipping.");

		return ResolveStreamPositionOriginal(a1);
	}
	// Loads from the data stream.
	bool LoadStreamNew(void* doesntSeemUsedlmao, uint8_t** a1, uint64_t** a2, uint64_t** a3)
	{
		return LoadStream(*a1, *a2, *a3);
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
						throw std::exception("MW6_PatchFile_RequestData(...) failed");
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
		// std::ofstream out("chunky.csv", std::ios::app);

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

		// out << decrypted << std::endl;

		return &StrBufferOffset;
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

		// Some assets in MW3 have names, we need it for logging below
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

		// TODO: Make a hash version of LinkXAssetEntry()
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
		if (assetType == 0x15)
		{
			// Get the image data offsets
			constexpr size_t imageDataPtrOffset = 0x38;
			constexpr size_t imageDataSizeOffset = 0x18;

			const auto gfxImage = result->Header;

			if (*(uint64_t*)(gfxImage + imageDataPtrOffset) != 0 && result->ExtendedData == nullptr)
			{
				const auto imageData = *(uint8_t**)(gfxImage + imageDataPtrOffset);
				const auto imageDataSize = (size_t)*(uint32_t*)(gfxImage + imageDataSizeOffset);

				result->ExtendedDataSize = imageDataSize;
				result->ExtendedData = std::make_unique<uint8_t[]>(result->ExtendedDataSize);
				result->ExtendedDataPtrOffset = imageDataPtrOffset;
				std::memcpy(result->ExtendedData.get(), imageData, result->ExtendedDataSize);
				*(uint64_t*)(gfxImage + imageDataPtrOffset) = (uint64_t)result->ExtendedData.get();

				ps::log::Log(ps::LogType::Verbose, "Resolved loaded data for image, Hash: 0x%llx, Type: 0x%llx.", hash, (uint64_t)assetType);
			}
		}

// #if PRIVATE_GRAM_GRAM
		if (assetType == 0x3B && DecryptString != nullptr)
		{
			char* str = *(char**)(result->Header + 8);
			if ((*str & 0xC0) == 0x80)
			{
				str = DecryptString(ps::CoDMW6Internal::StrDecryptBuffer.get(), ps::CoDMW6Internal::StrDecryptBufferSize, str, nullptr);
			}
			memcpy(*(char**)(result->Header + 8), str, strlen(str) + 1);
		}

		if (assetType == 0x45 && DecryptString != nullptr)
		{
			struct GSC_USEANIMTREE_ITEM
			{
				uint32_t num_address;
				uint32_t address;
			};

			struct GSC_ANIMTREE_ITEM
			{
				uint32_t num_address;
				uint32_t address_str1;
				uint32_t address_str2;
			};
			struct GSC_STRINGTABLE_ITEM
			{
				uint32_t string;
				uint8_t num_address;
				uint8_t type;
				uint16_t pad;
			};

			struct GSC_OBJ_8B
			{
				uint8_t magic[8];
				uint64_t name;
				uint16_t unk10;
				uint16_t animtree_use_count;
				uint16_t animtree_count;
				uint16_t devblock_string_count;
				uint16_t export_count;
				uint16_t fixup_count;
				uint16_t unk1C;
				uint16_t imports_count;
				uint16_t includes_count;
				uint16_t unk22;
				uint16_t string_count;
				uint16_t unk26;
				uint32_t checksum;
				uint32_t animtree_use_offset;
				uint32_t animtree_offset;
				uint32_t cseg_offset;
				uint32_t cseg_size;
				uint32_t devblock_string_offset;
				uint32_t export_offset;
				uint32_t fixup_offset;
				uint32_t size1;
				uint32_t import_table;
				uint32_t include_table;
				uint32_t size2;
				uint32_t string_table;
			};

			struct GscObjEntry
			{
				uint64_t name;
				int len;
				int padc;
				GSC_OBJ_8B* buffer;
			};

			const auto gscObj = (GscObjEntry*)result->Header;

			if (gscObj->len && gscObj->buffer && *(uint64_t*)gscObj->buffer == 0xa0d4353478b)
			{
				// encrypted script (VM 8B)
				GSC_OBJ_8B* script = gscObj->buffer;


				// string table
				GSC_STRINGTABLE_ITEM* strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(script->magic + script->string_table);

				for (int j = 0; j < script->string_count; j++)
				{
					char* str = (char*)(script->magic + strings->string);

					if ((*str & 0xC0) == 0x80)
					{
						const char* dstr = DecryptString(ps::CoDMW6Internal::StrDecryptBuffer.get(), ps::CoDMW6Internal::StrDecryptBufferSize, str, nullptr);
						memcpy(str, dstr, strlen(dstr) + 1);
					}

					strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(reinterpret_cast<uint32_t*>(&strings[1]) + strings->num_address);
				}

				// animtree tables

				GSC_ANIMTREE_ITEM* animt = reinterpret_cast<GSC_ANIMTREE_ITEM*>(script->magic + script->animtree_offset);

				for (int j = 0; j < script->animtree_count; j++)
				{
					{
						char* str = (char*)(script->magic + animt->address_str1);

						if ((*str & 0xC0) == 0x80)
						{
							const char* dstr = DecryptString(ps::CoDMW6Internal::StrDecryptBuffer.get(), ps::CoDMW6Internal::StrDecryptBufferSize, str, nullptr);
							memcpy(str, dstr, strlen(dstr) + 1);
						}
					}
					{
						char* str = (char*)(script->magic + animt->address_str2);

						if ((*str & 0xC0) == 0x80)
						{
							const char* dstr = DecryptString(ps::CoDMW6Internal::StrDecryptBuffer.get(), ps::CoDMW6Internal::StrDecryptBufferSize, str, nullptr);
							memcpy(str, dstr, strlen(dstr) + 1);
						}
					}

					animt = reinterpret_cast<GSC_ANIMTREE_ITEM*>(reinterpret_cast<uint32_t*>(&animt[1]) + animt->num_address);
				}

				GSC_USEANIMTREE_ITEM* animtu = reinterpret_cast<GSC_USEANIMTREE_ITEM*>(script->magic + script->animtree_use_offset);

				for (int j = 0; j < script->animtree_use_count; j++) {
					char* str = (char*)(script->magic + animtu->address);

					if ((*str & 0xC0) == 0x80)
					{
						const char* dstr = DecryptString(ps::CoDMW6Internal::StrDecryptBuffer.get(), ps::CoDMW6Internal::StrDecryptBufferSize, str, nullptr);
						memcpy(str, dstr, strlen(dstr) + 1);
					}

					animtu = reinterpret_cast<GSC_USEANIMTREE_ITEM*>(reinterpret_cast<uint32_t*>(&animtu[1]) + animtu->num_address);
				}

			}
		}

// #else
// 		switch (assetType)
// 		{
// 		// Kill localize 59
// 		case 0x3b:
// 		{
// 			char* str = *(char**)(result->Header + 8);
// 			std::memset(str, 0, strlen(str));
// 			*(uint64_t*)result->Header = 0;
// 			break;
// 		}
// 		// Kill Weapons 61
// 		case 0x3d:
// 		{
// 			if (*(char**)(result->Header + 32) != nullptr)
// 				std::memset(*(char**)(result->Header + 32), 0, 15784);
//
// 			std::memset(result->Header, 0, 1400);
// 			break;
// 		}
// 		// Kill Attachments 60
// 		case 0x3c:
// 		{
// 			std::memset(result->Header, 0, 2576);
// 			break;
// 		}
// 		// Kill Lua files 76
// 		case 0x4c:
// 		{
// 			*(uint64_t*)result->Header = 0;
// 			auto sizeOfRawFile = *(uint32_t*)(result->Header + 8);
// 			auto rawFileData = *(char**)(result->Header + 16);
// 			if (rawFileData != nullptr) std::memset(rawFileData, 0, sizeOfRawFile);
// 			break;
// 		}
// 		// Kill raw files 68
// 		case 0x44:
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
	void __fastcall FixUpXModelSurfsPtr(uint8_t* xmodel)
	{
		// Get the lods data offset
		const bool isSpGame = ps::Parasyte::GetCurrentHandler()->HasFlag("sp");
		const size_t lodsDataPtrOffset = isSpGame ? 272 : 152;

		size_t lodCount = *(uint8_t*)(xmodel + 18);
		uint8_t* lods = *(uint8_t**)(xmodel + lodsDataPtrOffset);

		for (size_t i = 0; i < lodCount; i++)
		{
			uint8_t* xmodelLod = lods + i * 72;
			uint8_t* xmodelSurfs = *(uint8_t**)xmodelLod;

			if (xmodelSurfs != nullptr)
			{
				*(uint64_t*)(xmodelLod + 8) = *(uint64_t*)(xmodelSurfs + 8);
			}
		}
	}
	uint64_t HashAsset(const char* data)
	{
		uint64_t result = 0x47F5817A5EF961BA;

		for (size_t i = 0; i < strlen(data); i++)
		{
			uint64_t value = tolower(data[i]);

			if (value == '\\')
				value = '/';

			result = 0x100000001B3 * (value ^ result);
		}

		return result & 0x7FFFFFFFFFFFFFFF;
	}
}

const std::string ps::CoDMW6Handler::GetName()
{
	return "Call of Duty: Modern Warfare 3 (2023)";
}

bool ps::CoDMW6Handler::Initialize(const std::string& gameDirectory)
{
	Configs.clear();
	GameDirectory = gameDirectory;

	if (!LoadConfigs("CoDMW6Handler"))
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

	PS_SETGAMEVAR(ps::CoDMW6Internal::GetXAssetHeaderSize);
	PS_SETGAMEVAR(ps::CoDMW6Internal::GetXAssetPoolSize);
	PS_SETGAMEVAR(ps::CoDMW6Internal::RequestPatchedData);
	PS_SETGAMEVAR(ps::CoDMW6Internal::RequestFastFileData);
	PS_SETGAMEVAR(ps::CoDMW6Internal::RequestPatchFileData);
	PS_SETGAMEVAR(ps::CoDMW6Internal::InitializePatch);
	PS_SETGAMEVAR(ps::CoDMW6Internal::RequestFastFileData);
	PS_SETGAMEVAR(ps::CoDMW6Internal::RequestFinalFileData);
	PS_SETGAMEVAR(ps::CoDMW6Internal::AssignFastFileMemoryPointers);
	PS_SETGAMEVAR(ps::CoDMW6Internal::InitAssetAlignmentInternal);
	PS_SETGAMEVAR(ps::CoDMW6Internal::AddAssetOffset);
	PS_SETGAMEVAR(ps::CoDMW6Internal::XAssetOffsetList);
	PS_SETGAMEVAR(ps::CoDMW6Internal::ZoneLoaderFlag);
	PS_SETGAMEVAR(ps::CoDMW6Internal::ParseFastFile);
	PS_SETGAMEVAR(ps::CoDMW6Internal::GetXAssetHash);
	PS_SETGAMEVAR(ps::CoDMW6Internal::GetXAssetName);
	PS_SETGAMEVAR(ps::CoDMW6Internal::XAssetTypeHasName);
	PS_SETGAMEVAR(ps::CoDMW6Internal::GetXAssetHeaderSize);
	PS_SETGAMEVAR(ps::CoDMW6Internal::GetXAssetTypeName);
	PS_SETGAMEVAR(ps::CoDMW6Internal::DecryptString);
	PS_SETGAMEVAR(ps::CoDMW6Internal::LoadStream);
	PS_SETGAMEVAR(ps::CoDMW6Internal::LoadStreamFuncPointers);

	PS_DETGAMEVAR(ps::CoDMW6Internal::ReadXFile);
	PS_DETGAMEVAR(ps::CoDMW6Internal::AllocateUniqueString);
	PS_DETGAMEVAR(ps::CoDMW6Internal::LinkGenericXAsset);
	PS_DETGAMEVAR(ps::CoDMW6Internal::LinkGenericXAssetEx);
	PS_DETGAMEVAR(ps::CoDMW6Internal::ReadPatchFile);
	PS_DETGAMEVAR(ps::CoDMW6Internal::ReadFastFile);

	PS_INTGAMEVAR(ps::CoDMW6Internal::ResolveStreamPosition, ps::CoDMW6Internal::ResolveStreamPositionOriginal);

	XAssetPoolCount   = 256;
	XAssetPools       = std::make_unique<XAssetPool[]>(XAssetPoolCount);
	Strings           = std::make_unique<char[]>(0x2000000);
	StringPoolSize    = 0;
	Initialized       = true;
	StringLookupTable = std::make_unique<std::map<uint64_t, size_t>>();

	// Game specific buffers.
	ps::CoDMW6Internal::XAssetAlignmentBuffer = std::make_unique<uint8_t[]>((size_t)65535 * 32);
	ps::CoDMW6Internal::StrDecryptBuffer = std::make_unique<uint8_t[]>(ps::CoDMW6Internal::StrDecryptBufferSize);

	Module.SaveCache(CurrentConfig->CacheName);
	LoadAliases(CurrentConfig->AliasesName);

	return true;
}

bool ps::CoDMW6Handler::Deinitialize()
{
	Module.Free();
	XAssetPoolCount        = 256;
	XAssetPools            = nullptr;
	Strings                = nullptr;
	StringPoolSize         = 0;
	Initialized            = false;
	StringLookupTable      = nullptr;
	FileSystem             = nullptr;
	GameDirectory.clear();

	// Clear game specific buffers
	ps::CoDMW6Internal::ResetPatchState(0, 0, 0, 0);
	ps::CoDMW6Internal::PatchFileState = nullptr;
	ps::CoDMW6Internal::XAssetAlignmentBuffer = nullptr;
	ps::CoDMW6Internal::StrDecryptBuffer = nullptr;

	ps::oodle::Clear();

	return true;
}

bool ps::CoDMW6Handler::IsValid(const std::string& param)
{
	return strcmp(param.c_str(), "mw6") == 0;
}

bool ps::CoDMW6Handler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
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

	uint8_t ffHeader[224]{};
	uint8_t fpHeader[504]{};

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
		std::memcpy(&ffHeader[0], &fpHeader[280], sizeof(ffHeader));
	}

	ps::CoDMW6Internal::ResetPatchState(
		*(uint64_t*)&fpHeader[16],
		*(uint64_t*)&fpHeader[24],
		*(uint64_t*)&fpHeader[32],
		*(uint64_t*)&fpHeader[48]);

	uint64_t* bufferSizes = (uint64_t*)&ffHeader[72];

	for (size_t i = 0; i < 17; i++)
	{
		if (bufferSizes[i] > 0)
		{
			ps::log::Log(ps::LogType::Verbose, "Allocating block: %llu of size: 0x%llx", i, bufferSizes[i]);
			ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[i].Initialize(bufferSizes[i], 4096);
		}
	}

	ps::CoDMW6Internal::LoadStreamFuncPointers[0][0] = (uint64_t)ps::CoDMW6Internal::LoadStreamNew;
	ps::CoDMW6Internal::FFDecompressor = std::make_unique<OodleDecompressorV3>(ffHandle, true);
	ps::CoDMW6Internal::FPDecompressor = std::make_unique<OodleDecompressorV3>(fpHandle, true);

	ps::CoDMW6Internal::InitAssetAlignment();
	ps::CoDMW6Internal::InitAssetAlignmentInternal();
	ps::CoDMW6Internal::AssignFastFileMemoryPointers(&ps::Parasyte::GetCurrentFastFile()->MemoryBlocks[0]);
	ps::CoDMW6Internal::ZoneLoaderFlag[0] = 1;
	ps::CoDMW6Internal::ParseFastFile(nullptr, ps::Parasyte::GetCurrentFastFile()->AssetList, ffName.c_str(), 0);

	ps::CoDMW6Internal::FFDecompressor = nullptr;
	ps::CoDMW6Internal::FPDecompressor = nullptr;

	// We must fix up any XModel surfs, as we may have overrode previous
	// temporary entries, etc.
	ps::Parasyte::GetCurrentHandler()->XAssetPools[9].EnumerateEntries([](ps::XAsset* asset)
	{
		ps::CoDMW6Internal::FixUpXModelSurfsPtr(asset->Header);
	});

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

bool DumpAliasesInternal()
{
	// AssetType: 0x3B
	constexpr size_t localizeEntryAssetPoolIdx = 0x3B;
	struct LocalizeEntry
	{
		uint64_t hash;
		char* value;
	};

	// Store localizeEntries as map:(hash - value)
	std::map<size_t, std::string> localizeEntries;
	// Read localizeEntries
	ps::Parasyte::GetCurrentHandler()->XAssetPools[localizeEntryAssetPoolIdx].EnumerateEntries([&](ps::XAsset* asset)
	{
		auto entry = (LocalizeEntry*)asset->Header;
		localizeEntries[entry->hash] = entry->value;

		//ps::log::Print("MAIN", "hash: %llx", entry->hash);
		//ps::log::Print("MAIN", "value: %s", entry->value);
	});

	// Read weapon asset data
	constexpr size_t weaponAssetPoolIdx = 0x3D;

	json resultJson;
	ps::Parasyte::GetCurrentHandler()->XAssetPools[weaponAssetPoolIdx].EnumerateEntries([&](ps::XAsset* asset)
	{
		const std::string wpnAssetName = ps::CoDMW6Internal::GetXAssetName(weaponAssetPoolIdx, asset->Header);

		// For singe asset test
		// if (wpnAssetName != /*"jup_jp36_ar_anov94_mp"*/"jup_cp15_lm_mkilo3_mp")
		// {
		// 	return;
		// }

		json wpnJson;
		std::unordered_set<std::string> xmodelNames;
		std::unordered_set<size_t> xmodelHashes;

		auto header = (size_t)asset->Header;

		auto szInternalNamePtr = header + 8;
		auto szDisplayNamePtr = header + 64;
		auto _attSlotPtr = header + 88;

		auto szInternalName = *(char**)(szInternalNamePtr);
		auto szDisplayName = *(char**)(szDisplayNamePtr);

		for (size_t i = 0; i < 17; ++i) // 17 is not sure, just guess
		{
			auto attSlotPtr = _attSlotPtr + i * 16;

			auto attCount = *(size_t*)attSlotPtr;
			auto _attsPtr = *(size_t*)(attSlotPtr + 8);

			if (!_attsPtr || !attCount)
			{
				continue;
			}

			for (size_t j = 0; j < attCount; ++j)
			{
				auto attsPtr = _attsPtr + j * 8;

				auto att = *(size_t*)attsPtr;

				// Read Base WM
				if (auto baseWmPtrPtr = *(size_t*)(att + 376))
				{
					if (auto baseWmPtr = *(size_t*)baseWmPtrPtr)
					{
						auto wmName = *(char**)(baseWmPtr + 8);
						//ps::log::Print("MAIN", "wm: %s", wmName);
						xmodelNames.insert(wmName);
						xmodelHashes.insert(*(size_t*)baseWmPtr);
					}
				}

				// Read Base VM
				if (auto baseVmPtrPtr = *(size_t*)(att + 384))
				{
					if (auto baseVmPtr = *(size_t*)baseVmPtrPtr)
					{
						auto vmName = *(char**)(baseVmPtr + 8);
						//ps::log::Print("MAIN", "vm: %s", vmName);
						xmodelNames.insert(vmName);
						xmodelHashes.insert(*(size_t*)baseVmPtr);
					}
				}

				auto bpCount = *(uint32_t*)(att + 48);
				auto _bpsPtr = *(size_t*)(att + 56);

				//ps::log::Print("MAIN", "att: %llx", att);
				//ps::log::Print("MAIN", "bpCount: %d", bpCount);
				//ps::log::Print("MAIN", "_bpsPtr: %llx", _bpsPtr);

				if (!bpCount || !_bpsPtr)
				{
					continue;
				}

				for (size_t k = 0; k < bpCount; ++k)
				{
					auto bpsPtr = _bpsPtr + k * 8;
					//ps::log::Print("MAIN", "k: %lld", k);

					auto bp = *(size_t*)bpsPtr;

					if (bp == 0)
					{
						continue;
					}

					auto wmPtr = *(size_t*)(bp + 16);
					auto vmPtr = *(size_t*)(bp + 40);

					if (wmPtr != 0)
					{
						auto wmName = *(char**)(wmPtr + 8);
						//ps::log::Print("MAIN", "wm: %s", wmName);
						xmodelNames.insert(wmName);
						xmodelHashes.insert(*(size_t*)wmPtr);
					}

					if (vmPtr != 0)
					{
						auto vmName = *(char**)(vmPtr + 8);
						//ps::log::Print("MAIN", "vm: %s", vmName);
						xmodelNames.insert(vmName);
						xmodelHashes.insert(*(size_t*)vmPtr);
					}
				}
			}
		}

		std::unordered_set<std::string> ffNames;
		constexpr size_t xmodelAssetPoolIdx = 0x9;
		auto xmodelPool = &ps::Parasyte::GetCurrentHandler()->XAssetPools[xmodelAssetPoolIdx];
		for (const auto& hash : xmodelHashes)
		{
			auto xmodelAsset = xmodelPool->FindXAssetEntry(hash, xmodelAssetPoolIdx);

			// Failed to find loaded xmodel asset
			if (!xmodelAsset)
			{
				continue;
			}

			const auto& ffName = xmodelAsset->Owner->Name;
			if (xmodelAsset->Temp || ffNames.contains(ffName))
			{
				continue;
			}

			const auto xmodelName = ps::CoDMW6Internal::GetXAssetName((uint32_t)xmodelAssetPoolIdx, xmodelAsset->Header);

			// ps::log::Print("MAIN", "[ff: %s] - [xmodel: %s]", ffName.c_str(), xmodelName);
			ffNames.insert(ffName);
		}

		std::string aliasName = szInternalName ? szInternalName : "*None*";
		if (szDisplayName)
		{
			auto pair = localizeEntries.find(ps::CoDMW6Internal::HashAsset(szDisplayName));
			if (pair != localizeEntries.end())
			{
				aliasName = pair->second;
			}
		}

		wpnJson["alias"] = aliasName;
		wpnJson["name"] = szInternalName;
		wpnJson["fast_files"] = ffNames;

		resultJson.emplace_back(wpnJson);
	});

	std::ofstream out_file("Data/Aliases/ModernWarfare6Aliases.json");
	out_file << resultJson.dump(4);
	out_file.close();

	return true; 
}

bool ps::CoDMW6Handler::DumpAliases()
{
	DumpAliasesInternal();

	return true;
}

bool ps::CoDMW6Handler::CleanUp()
{
	ps::CoDMW6Internal::FFDecompressor = nullptr;
	ps::CoDMW6Internal::FPDecompressor = nullptr;
	ps::CoDMW6Internal::ResetPatchState(0, 0, 0, 0);
	return false;
}

char* ps::CoDMW6Handler::DecryptString(char* str)
{
	if (!ps::CoDMW6Internal::DecryptString)
	{
		throw std::exception("Can't find DecryptString function.");
	}

	if ((*str & 0xC0) == 0x80)
	{
		const char* dstr = ps::CoDMW6Internal::DecryptString(ps::CoDMW6Internal::StrDecryptBuffer.get(), ps::CoDMW6Internal::StrDecryptBufferSize, str, nullptr);
		memcpy(str, dstr, strlen(dstr) + 1);
	}

	return str;
}

std::string ps::CoDMW6Handler::GetFileName(const std::string& name)
{
	return (CurrentConfig->FilesDirectory.empty()) ? (name) : (CurrentConfig->FilesDirectory + "/" + name);
}

const std::string ps::CoDMW6Handler::GetShorthand()
{
	return "mw6";
}

PS_CINIT(ps::GameHandler, ps::CoDMW6Handler, ps::GameHandler::GetHandlers());
#endif