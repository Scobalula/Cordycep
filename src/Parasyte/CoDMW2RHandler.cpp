#include "pch.h"
#if _WIN64
#include "CascLib.h"
#include "Parasyte.h"
#include "CoDMW2RHandler.h"
// #include "XAssetPackageRef.h"
#include "lz4.h"
#include <intrin.h>
#include <array>

// The storage handle.
HANDLE MW2CR_StorageHandle = NULL;
// The fast file file handle.
HANDLE MW2CR_FastFileHandle = NULL;
// Whether or not the current file is secure.
int MW2CR_FileIsSecure = 0;
// Current compressor
int MW2CR_FileCompression = -1;
// Current Fast File 
ps::FastFile* MW2CR_CurrentFastFile = nullptr;
// Sound Files
std::array<HANDLE, 512> MW2CR_SoundFiles;
// Localized Sound Files
std::array<HANDLE, 512> MW2CR_LocalizedSoundFiles;

const char* (__fastcall* MW2CR_DB_GetXAssetName)(void* xassetHeader);

const char* (__fastcall* MW2CR_DB_SetXAssetName)(void* xassetHeader, const char* name);

const size_t (__fastcall* MW2CR_MW2CR_DB_GetXAssetTypeSizeInternal)(uint32_t xassetType);

struct MW2CR_XImageData
{
	uint16_t Locale;
	uint16_t PackageIndex;
	uint32_t Checksum;
	uint64_t Offset;
	uint64_t Size;
};

// Blank Graphics Dvar Buffer
char MW2CR_MW2CR_GraphicsDvarBuffer[512]{};
// Graphics Related Dvar
char** MW2CR_GraphicsDvar = nullptr;
// Images Count
uint32_t* MW2CR_ImageCount = nullptr;
// Current Images Index
uint32_t MW2CR_ImageIndex = 0;
// Images Buffer
MW2CR_XImageData* MW2CR_ImagesBuffer = nullptr;

// Zone Memory Block
struct MW2CR_XBlock
{
	void* data;
	size_t size;
};

// Zone Memory
struct MW2CR_XZoneMemory
{
	MW2CR_XBlock blocks[7];
};



// Requests sound file data for loaded audio.
void MW2CR_DB_RequestSoundFileData(void* ptr, uint16_t index, size_t fileOffset, size_t size, const char* name, const bool isLocalized)
{
	if (!ptr)
		return;
	if (fileOffset == 0 || size == 0)
		return;

	HANDLE handle = isLocalized ? MW2CR_LocalizedSoundFiles[index] : MW2CR_SoundFiles[index];

	if (handle == NULL)
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

		if (!CascOpenFile(MW2CR_StorageHandle, soundFilePath.c_str(), NULL, NULL, &handle))
		{
			handle = INVALID_HANDLE_VALUE;
		}

		if(isLocalized)
			MW2CR_LocalizedSoundFiles[index] = handle;
		else
			MW2CR_SoundFiles[index] = handle;
	}

	if (handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		if (CascSetFilePointer64(handle, fileOffset, NULL, FILE_BEGIN))
		{
			if (!CascReadFile(handle, ptr, (DWORD)size, NULL))
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

// Seeks into further into the file from where we are.
void MW2CR_DB_Seek(size_t v)
{
	LARGE_INTEGER Input{};
	Input.QuadPart = v;

	if (!CascSetFilePointer64(MW2CR_FastFileHandle, v, NULL, FILE_CURRENT))
	{
		throw std::exception("Failed to seek in fast file.");
	}
}

// Seeks into further into the file from where we are.
void MW2CR_DB_SetPosition(size_t v)
{
	if (!CascSetFilePointer64(MW2CR_FastFileHandle, v, NULL, FILE_BEGIN))
	{
		throw std::exception("Failed to seek in fast file.");
	}
}

// Reads from the file.
void MW2CR_DB_Read(void* pos, size_t size)
{
	DWORD bytesRead = 0;

	if (!CascReadFile(MW2CR_FastFileHandle, pos, (DWORD)size, &bytesRead))
	{
		throw std::exception("Failed to read data from the fast file.");
	}

	if (bytesRead != size)
	{
		throw std::exception("Failed to read from fast file.");
	}
}

// Gets the current position of the file.
size_t MW2CR_DB_Position()
{
	size_t Input{};

	if (!CascSetFilePointer64(MW2CR_FastFileHandle, { 0 }, &Input, FILE_CURRENT))
	{
		throw std::exception("Failed to request position from fast file.");
	}

	return Input;
}

// Whether or not the first auth block was consumed.
bool MW2CR_AuthBlockConsumed = false;
// Distance to the next hash block
size_t MW2CR_DistanceToNextHashBlock = 0;

// Reads uncompressed data from the file while accounting for hash blocks.
void MW2CR_DB_ReadXFileUncompressed(void* pos, size_t size)
{
	if (!MW2CR_FileIsSecure)
	{
		MW2CR_DB_Read(pos, size);
	}
	else
	{
		if (!MW2CR_AuthBlockConsumed)
		{
			MW2CR_DB_Seek(0x8000);
			MW2CR_DistanceToNextHashBlock = 0x800000;
			MW2CR_AuthBlockConsumed = true;
		}

		char* ptrInternal = (char*)pos;
		size_t dataConsumed = 0;

		if (MW2CR_DistanceToNextHashBlock < size)
		{
			MW2CR_DB_Read(ptrInternal, MW2CR_DistanceToNextHashBlock);
			MW2CR_DB_Seek(0x4000);
			dataConsumed += MW2CR_DistanceToNextHashBlock;
			ptrInternal += dataConsumed;
			MW2CR_DistanceToNextHashBlock = 0x800000;
		}

		auto trail = size - dataConsumed;

		MW2CR_DB_Read(ptrInternal, trail);
		MW2CR_DistanceToNextHashBlock -= trail;
	}
}

// The current compressed buffer
std::unique_ptr<uint8_t[]> MW2CR_CompressedBuffer;
// The current offset within the compressed buffer
uint32_t MW2CR_CompressedBufferOffset = 0;
// The size of the compressed buffer
uint32_t MW2CR_CompressedBufferSize = 0;
// The capacity of the compressed Buffer
uint32_t MW2CR_CompressedBufferCapacity = 0;

// The current decompressed buffer
std::unique_ptr<uint8_t[]> MW2CR_DecompressedBuffer;
// The current offset within the decompressed buffer
uint32_t MW2CR_DecompressedBufferOffset = 0;
// The size of the decompressed buffer
uint32_t MW2CR_DecompressedBufferSize = 0;
// The capacity of the decompressed Buffer
uint32_t MW2CR_DecompressedBufferCapacity = 0;
// The current fast file bloc.
uint32_t MW2CR_CurrentBlock = 0;

// The current position in the raw file data.
size_t MW2CR_CurrentRawFilePosition = 0;
// The size of the compressed file.
size_t MW2CR_CompressedFileSize = 0;
// The current position in the compressed file.
size_t MW2CR_CompressedFilePosition = 0;

void MW2CR_DB_AllocFastFileHeaders()
{

}

// Resets all data.
void MW2CR_DB_Reset()
{
	MW2CR_ImageIndex                    = 0;
	MW2CR_CurrentBlock				    = 0;
	MW2CR_DecompressedBuffer			= nullptr;
	MW2CR_DecompressedBufferOffset	    = 0;
	MW2CR_DecompressedBufferSize		= 0;
	MW2CR_DecompressedBufferCapacity	= 0;
	MW2CR_CompressedBuffer			    = nullptr;
	MW2CR_CompressedBufferOffset	    = 0;
	MW2CR_CompressedBufferSize		    = 0;
	MW2CR_CompressedBufferCapacity	    = 0;
	MW2CR_CurrentRawFilePosition		= 0;
	MW2CR_CurrentFastFile				= nullptr;
	MW2CR_DistanceToNextHashBlock		= 0;
	MW2CR_AuthBlockConsumed             = true;
	MW2CR_FileIsSecure                  = 0;
	MW2CR_CompressedFilePosition        = 0;
	MW2CR_CompressedFileSize			= 0;

	CascCloseFile(MW2CR_FastFileHandle);
	MW2CR_FastFileHandle = NULL;
}

void MW2CR_DB_ReadXFile(void* ptr, size_t size)
{
	char* ptrInternal = (char*)ptr;
	size_t remaining = size;
	MW2CR_CurrentRawFilePosition += size;

	while (true)
	{
		auto cacheRemaining = MW2CR_DecompressedBufferSize - MW2CR_DecompressedBufferOffset;

		if (cacheRemaining > 0)
		{
			auto toCopy = min(cacheRemaining, remaining);
			std::memcpy(ptrInternal, &MW2CR_DecompressedBuffer[MW2CR_DecompressedBufferOffset], toCopy);

			ptrInternal += toCopy;
			MW2CR_DecompressedBufferOffset += (uint32_t)toCopy;
			remaining -= toCopy;
		}

		if (remaining <= 0)
			break;

		if (MW2CR_CurrentBlock == 0)
		{
			char info[12]{};
			MW2CR_DB_ReadXFileUncompressed(info, sizeof(info));
		}

		MW2CR_DB_ReadXFileUncompressed(&MW2CR_CompressedBufferSize, sizeof(MW2CR_CompressedBufferSize));
		MW2CR_DB_ReadXFileUncompressed(&MW2CR_DecompressedBufferSize, sizeof(MW2CR_DecompressedBufferSize));

		// Realloc if required
		if (MW2CR_CompressedBufferSize > MW2CR_CompressedBufferCapacity)
		{
			MW2CR_CompressedBufferCapacity = ((size_t)MW2CR_CompressedBufferSize + 4095) & 0xFFFFFFFFFFFFF000;
			if (MW2CR_CompressedBufferCapacity < 0x10000)
				MW2CR_CompressedBufferCapacity = 0x10000;
			MW2CR_CompressedBuffer = std::make_unique<uint8_t[]>(MW2CR_CompressedBufferCapacity);
		}
		if (MW2CR_DecompressedBufferSize > MW2CR_DecompressedBufferCapacity)
		{
			MW2CR_DecompressedBufferCapacity = ((size_t)MW2CR_DecompressedBufferSize + 4095) & 0xFFFFFFFFFFFFF000;
			if (MW2CR_DecompressedBufferCapacity < 0x10000)
				MW2CR_DecompressedBufferCapacity = 0x10000;
			MW2CR_DecompressedBuffer = std::make_unique<uint8_t[]>(MW2CR_DecompressedBufferCapacity);
		}

		MW2CR_CompressedBufferOffset = 0;
		MW2CR_DecompressedBufferOffset = 0;

		MW2CR_DB_ReadXFileUncompressed(&MW2CR_CompressedBuffer[0], ((size_t)MW2CR_CompressedBufferSize + 0x3) & 0xFFFFFFFFFFFFFFC);

		auto result = LZ4_decompress_safe(
			(const char*)MW2CR_CompressedBuffer.get(),
			(char*)MW2CR_DecompressedBuffer.get(),
			MW2CR_CompressedBufferSize,
			MW2CR_DecompressedBufferSize);

		if (result != MW2CR_DecompressedBufferSize)
		{
			ps::log::Log(ps::LogType::Error, "Failed to decompress data.");
			throw std::exception();
		}

		MW2CR_CurrentBlock++;
	}
}

void MW2CR_DB_AllocXZoneMemory(unsigned __int64* blockSize, const char* filename, MW2CR_XZoneMemory* zoneMem)
{
	for (int i = 0; i < 7; i++)
	{
		MW2CR_CurrentFastFile->MemoryBlocks[i].Initialize(blockSize[i], 0x1000);

		zoneMem->blocks[i].size = MW2CR_CurrentFastFile->MemoryBlocks[i].MemorySize;
		zoneMem->blocks[i].data = MW2CR_CurrentFastFile->MemoryBlocks[i].Memory;
	}
}

void MW2CR_DB_SetCompressor(int compression)
{
	MW2CR_FileCompression = compression;
}

void MW2CR_DB_InflateInit(int fileIsSecure)
{
	MW2CR_FileIsSecure = fileIsSecure;
}

void MW2CR_DB_CalculateFileSize()
{
	//LARGE_INTEGER Output{};

	//if (!SetFilePointerEx(MW2CR_FastFileHandle, { 0 }, &Output, FILE_END))
	//{
	//	throw std::exception("Failed to seek in fast file.");
	//}

	//MW2CR_CompressedFileSize = Output.QuadPart;
	//MW2CR_DB_SetPosition(0);
}

typedef uint32_t MW2CR_XAssetType;

struct MW2CR_XAssetHeader
{
	void* Header;
};

struct MW2CR_XAsset
{
	MW2CR_XAssetType type;
	MW2CR_XAssetHeader header;
};

struct MW2CR_XAssetEntry
{
	MW2CR_XAsset asset;
	char zoneIndex;
	volatile char inuseMask;
	unsigned int nextHash;
	unsigned int nextOverride;
	unsigned int nextPoolEntry;
};

MW2CR_XAssetEntry DB_LinkedXAsset{};

size_t MW2CR_DB_GetXAssetTypeSize(MW2CR_XAssetType xassetType)
{
	auto result = MW2CR_MW2CR_DB_GetXAssetTypeSizeInternal(xassetType);

	// Add on extra for asset specific data that
	// the game would usually store in a table
	switch (xassetType)
	{
	case 16:
	{
		result += 128;
		break;
	}
	}

	return result;
}

MW2CR_XAssetEntry* DB_LinkMW2CR_XAssetEntry(MW2CR_XAssetType xassetType, MW2CR_XAssetHeader* asset)
{
	MW2CR_XAsset xasset
	{
		xassetType,
		*asset
	};

	auto name = MW2CR_DB_GetXAssetName(&xasset);
	auto pool = &ps::Parasyte::GetCurrentHandler()->XAssetPools[xassetType];
	auto temp = name[0] == ',';
	auto size = MW2CR_DB_GetXAssetTypeSize(xassetType);

	// Ensure pool is initialized
	pool->Initialize(size, 16384);

	if (temp)
	{
		MW2CR_DB_SetXAssetName(&xasset, name + 1);
		name = MW2CR_DB_GetXAssetName(&xasset);
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
				MW2CR_CurrentFastFile,
				(uint8_t*)asset->Header,
				temp);
		}
		else
		{
			result->Override(
				MW2CR_CurrentFastFile,
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
			MW2CR_CurrentFastFile,
			(uint8_t*)asset->Header,
			temp);
	}

	// Loggary for Stiggary
	ps::log::Log(ps::LogType::Verbose, "%s Type: %i @ %llu @ %llu", name, xassetType, (uint64_t)result->Header, (uint64_t)result->Temp);

	DB_LinkedXAsset.asset.type = xassetType;
	DB_LinkedXAsset.asset.header.Header = result->Header;

	auto header = result->Header;

	if (!temp)
	{
		switch (xassetType)
		{
		case 16:
		{
			// Check are we a streamed image, if we are, we need to grab the streaming
			// info that was loaded from the fast file's header. The game resolves this 
			// later so we must do it now.
			if (*(uint8_t*)(header + 0x35) != 0)
			{
				// No image table but we have images, so attempt locate from another image?
				// Haven't the slightest clue how the game accounts for this rn but we'll do it
				// like Harry's marriage.
				if (*MW2CR_ImageCount == 0)
				{
					auto xasset = result->FindChildOfFastFile(MW2CR_CurrentFastFile->Parent);

					if (xasset != nullptr)
					{
						std::memcpy(header + 104 + sizeof(MW2CR_XImageData), xasset->Header + 104 + sizeof(MW2CR_XImageData), sizeof(MW2CR_XImageData) * 4);
					}
				}
				else
				{
					for (size_t i = 0; i < 4; i++)
					{
						// Should never happen on a valid Fast File
						if (MW2CR_ImageIndex >= *MW2CR_ImageCount)
							throw std::exception("Invalid image index, corrupt PAK table in fast file.");

						auto image = MW2CR_ImagesBuffer[MW2CR_ImageIndex++];
						// Copy to end of the header so that tools can read directly from it
						std::memcpy(header + 104 + sizeof(MW2CR_XImageData) * i, &image, sizeof(MW2CR_XImageData));
						ps::log::Log(ps::LogType::Verbose, "Mip Map Pkg: %llu @ 0x%llx", (uint64_t)image.PackageIndex, image.Offset);
					}
				}
			}
			else
			{
				if (*(uint64_t*)(header + 56) != 0)
				{
					result->ExtendedDataSize = *(uint32_t*)(header + 40);
					result->ExtendedData = std::make_unique<uint8_t[]>(result->ExtendedDataSize);
					result->ExtendedDataPtrOffset = 56;
					std::memcpy(result->ExtendedData.get(), *(void**)(header + 56), result->ExtendedDataSize);
					*(void**)(header + 56) = result->ExtendedData.get();
				}
			}
			break;
		}
		case 23:
		{
			MW2CR_DB_RequestSoundFileData(
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

	return &DB_LinkedXAsset;
}

void* DB_FindMW2CR_XAssetHeader(MW2CR_XAssetType xassetType, const char* name, int allowCreateDefault)
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
			(uint32_t)MW2CR_DB_GetXAssetTypeSize(xassetType),
			MW2CR_CurrentFastFile,
			nullptr,
			true);

		MW2CR_XAsset asset
		{
			xassetType,
			result->Header
		};
		MW2CR_DB_SetXAssetName(&asset, name);

		return result->Header;
	}

	return nullptr;
}

__int64 MW2CR_DB_GetString(const char* str, unsigned int user)
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

void* (__cdecl* MW2CR_DB_LoadXFile)(const void* a, const void* b, const void* c, const bool d);

__int64 __fastcall MW2CR_DB_XModelSurfsFixup(__int64 a1)
{
	__int16 v1; // r8
	__int64 result; // rax
	int v3; // er10
	uint64_t* v4; // rdx
	__int64 v5; // r9

	v1 = 0;
	result = 65533i64;
	*(uint16_t*)(a1 + 556) &= ~2u;
	v3 = 0;
	if (*(char*)(a1 + 552) > 0)
	{
		v4 = (uint64_t*)(a1 + 168);
		do
		{
			v5 = *(v4 - 5);
			*((__m128*)v4 - 2) = *(__m128*)(v5 + 20);
			*((__m128*)v4 - 1) = *(__m128*)(v5 + 36);
			*v4 = *(uint64_t*)(v5 + 8);
			*((uint16_t*)v4 - 22) = *(uint16_t*)(v5 + 16);
			*((uint16_t*)v4 - 21) = v1;
			++v3;
			v1 += *(uint16_t*)(v5 + 16);
			v4 += 9;
			result = (unsigned int)*(char*)(a1 + 552);
		} while (v3 < (int)result);
	}
	*(uint8_t*)(a1 + 10) = (uint8_t)v1;
	return result;
}

bool MW2CR_IsCascFileValid(HANDLE handle)
{
	uint64_t Result = 0;
	CascGetFileSize64(handle, &Result);
	return Result > 0;
}

const std::string ps::CoDMW2RHandler::GetShorthand()
{
	return "mw2cr";
}

const std::string ps::CoDMW2RHandler::GetName()
{
	return "Call of Duty: Modern Warfare 2 Campaign Remastered";
}

bool ps::CoDMW2RHandler::GetFiles(const std::string& pattern, std::vector<std::string>& results)
{
	if (MW2CR_StorageHandle != NULL)
	{
		HANDLE fileHandle;
		CASC_FIND_DATA findData;

		fileHandle = CascFindFirstFile(MW2CR_StorageHandle, pattern.c_str(), &findData, NULL);

		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (findData.bFileAvailable)
				{
					std::string_view fileName = findData.szFileName;

					if (fileName.ends_with(".ff") &&
						fileName.find('\\') == std::string::npos &&
						fileName.find('/') == std::string::npos)
					{
						results.emplace_back(fileName.substr(0, fileName.find(".ff")));
					}
				}
			} while (CascFindNextFile(fileHandle, &findData));
		}

		CascFindClose(fileHandle);
		return true;
	}

	return false;
}

const std::vector<std::string>& ps::CoDMW2RHandler::GetCommonFileNames() const
{
	static std::vector<std::string> results
	{
		"code_post_gfx",
		"code_pre_gfx",
		"common",
		"common_ignore",
	};

	return results;
}

bool ps::CoDMW2RHandler::Initialize(const std::string& gameDirectory)
{
	ps::log::Log(ps::LogType::Normal, "Attempting to initialize Call of Duty: Modern Warfare 2 Campaign Remastered....");
	ps::log::Log(ps::LogType::Normal, "Attempting to open directory: %s", gameDirectory.c_str());

	// Attempt to open game storage first
	if (!CascOpenStorage(gameDirectory.c_str(), 0, &MW2CR_StorageHandle))
	{
		ps::log::Log(ps::LogType::Error, "CascOpenStorage(gameDirectory.c_str(), 0, &StorageHandle) returned: %llx", GetLastError());
		ps::log::Log(ps::LogType::Error, "Failed to open Casc Storage, ensure the game path you provided is correct and the platform is not in the middle of an update.", gameDirectory);
		ps::log::Log(ps::LogType::Error, "If your game has recently completed an update, you may need to launch it to finalize any in-game updates.", gameDirectory);
		return false;
	}

	if (!std::filesystem::exists(gameDirectory))
	{
		ps::log::Log(ps::LogType::Error, "Failed to locate Call of Duty: Modern Warfare 2 Campaign Remastered's at: %s", gameDirectory.c_str());
		return false;
	}

	ps::log::Log(ps::LogType::Normal, "Successfully opened directory: %s", gameDirectory.c_str());

	if (!Module.Load("Data\\Dumps\\MW2CR_dump.exe"))
	{
		ps::log::Log(ps::LogType::Error, "Failed to load Call of Duty: Modern Warfare 2 Campaign Remastered's Module: Data\\Dumps\\MW2CR_dump.exe");
		ps::log::Log(ps::LogType::Error, "Please refer to the Wiki for information producing a game dump.");
		return false;
	}

	Module.LoadCache("Data\\Dumps\\MW2CR_dump.cache");

	Pattern pattern;

	pattern.Update("48 8B D6 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 48");
	if (!Module.FindVariableAddress(
		&MW2CR_DB_LoadXFile,
		pattern,
		4,
		"IW8::Var0",
		ps::ScanType::FromEndOfData)) return false;
	pattern.Update("75 ?? 48 8B CB E8 ?? ?? ?? ?? 48 8B C8 48 8B D5 E8");
	if (!Module.FindVariableAddress(
		&MW2CR_DB_GetXAssetName,
		pattern,
		6,
		"IW8::Var1",
		ps::ScanType::FromEndOfData)) return false;
	pattern.Update("48 8B CD E8 ?? ?? ?? ?? 0F B6 45 12 A8 08 74");
	if (!Module.FindVariableAddress(
		&MW2CR_DB_SetXAssetName,
		pattern,
		4,
		"IW8::Var2",
		ps::ScanType::FromEndOfData)) return false;
	pattern.Update("B9 07 00 00 00 E8 ?? ?? ?? ?? 48 8B 4C 24 28 48");
	if (!Module.FindVariableAddress(
		&MW2CR_MW2CR_DB_GetXAssetTypeSizeInternal,
		pattern,
		6,
		"IW8::Var3",
		ps::ScanType::FromEndOfData)) return false;
	pattern.Update("48 89 05 ?? ?? ?? ?? B9 EF 7A 3B 3B E8");
	if (!Module.FindVariableAddress(
		&MW2CR_GraphicsDvar,
		pattern,
		3,
		"IW8::Var4",
		ps::ScanType::FromEndOfData)) return false;
	pattern.Update("E8 ?? ?? ?? ?? 8B 05 ?? ?? ?? ?? 3D 80 BB 00 00");
	if (!Module.FindVariableAddress(
		&MW2CR_ImageCount,
		pattern,
		7,
		"IW8::Var5",
		ps::ScanType::FromEndOfData)) return false;
	pattern.Update("74 23 48 8D 35 ?? ?? ?? ?? 48 8D 0C 5B BA 18 00 00 00");
	if (!Module.FindVariableAddress(
		&MW2CR_ImagesBuffer,
		pattern,
		5,
		"IW8::Var6",
		ps::ScanType::FromEndOfData)) return false;

	// If we don't find any of these, it's a hard fail like Harry's modding career.
	// Variables
	pattern.Update("BA 04 00 00 00 48 8D 4D C3 E8 ?? ?? ?? ?? 44");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		10,
		"MWR::Det0",
		ps::ScanType::FromEndOfData), (uintptr_t)&MW2CR_DB_ReadXFileUncompressed)) return false;
	pattern.Update("41 8D 50 28 E8 ?? ?? ?? ?? B9 03 00 00 00 E8");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		5,
		"MWR::Det1",
		ps::ScanType::FromEndOfData), (uintptr_t)&MW2CR_DB_ReadXFile)) return false;
	pattern.Update("0F 95 C1 E8 ?? ?? ?? ?? 45 33 C0");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		4,
		"MWR::Det2",
		ps::ScanType::FromEndOfData), (uintptr_t)&MW2CR_DB_InflateInit)) return false;
	pattern.Update("0F B6 4D AF E8 ?? ?? ?? ?? BA 04 00 00 00");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		5,
		"MWR::Det3",
		ps::ScanType::FromEndOfData), (uintptr_t)&MW2CR_DB_SetCompressor)) return false;
	pattern.Update("48 8D 4D F7 E8 ?? ?? ?? ?? 49 8B CE");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		5,
		"MWR::Det4",
		ps::ScanType::FromEndOfData), (uintptr_t)&MW2CR_DB_AllocXZoneMemory)) return false;
	pattern.Update("BA 04 00 00 00 E8 ?? ?? ?? ?? 48 63 C8");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		6,
		"MWR::Det5",
		ps::ScanType::FromEndOfData), (uintptr_t)&MW2CR_DB_GetString)) return false;
	pattern.Update("0F 11 05 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		8,
		"MWR::Det6",
		ps::ScanType::FromEndOfData), (uintptr_t)&DB_LinkMW2CR_XAssetEntry)) return false;
	//pattern.Update("B8 01 00 00 00 41 8D 48 23 E8 ?? ?? ?? ?? 0F B6 48 10 48 8D");
	//if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
	//	pattern,
	//	10,
	//	"MWR::Det7",
	//	ps::ScanType::FromEndOfData), (uintptr_t)&DB_FindMW2CR_XAssetHeader)) return false;
	pattern.Update("48 8B 4C 29 08 33 D2 E8 ?? ?? ?? ?? 48 83 C3 08");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		8,
		"MWR::Det7",
		ps::ScanType::FromEndOfData), (uintptr_t)&MW2CR_DB_XModelSurfsFixup)) return false;

	pattern.Update("8D 48 01 E8 ?? ?? ?? ?? 48 8B 5C 24 30");
	if (!Module.NullifyFunction(
		pattern,
		4,
		"MWR::Null1",
		true,
		true)) return false;
	pattern.Update("C7 05 ?? ?? ?? ?? 01 00 00 00 E8 ?? ?? ?? ?? 8B");
	if (!Module.NullifyFunction(
		pattern,
		11,
		"MWR::Null2",
		true,
		true)) return false;
	pattern.Update("48 8B 52 08 E8 ?? ?? ?? ?? 48 89 1D ?? ?? ?? ?? E8");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null3",
		true,
		true)) return false;
	pattern.Update("48 8B 52 10 E8 ?? ?? ?? ?? 48 89 1D ?? ?? ?? ?? E8");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null4",
		true,
		true)) return false;
	pattern.Update("4D 8B 40 38 E8 ?? ?? ?? ?? 48 89 1D ?? ?? ?? ?? E8");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null5",
		true,
		true)) return false;
	pattern.Update("48 8B 52 18 44 0F AF C0 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null6",
		true,
		true)) return false;
	pattern.Update("48 8B 52 30 44 0F AF C0 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null7",
		true,
		true)) return false;
	pattern.Update("48 8B 52 20 44 8D 04 40 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null8",
		true,
		true)) return false;
	pattern.Update("48 8B 52 68 41 C1 E1 05 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null9",
		true,
		true)) return false;
	pattern.Update("48 8B 52 78 41 C1 E0 02 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null10",
		true,
		true)) return false;
	pattern.Update("41 C1 E0 02 E8 ?? ?? ?? ?? 48 89 3D ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null11",
		true,
		true)) return false;
	pattern.Update("41 B8 18 00 00 00 33 C9 E8 ?? ?? ?? ?? 48 8B CB E8");
	if (!Module.NullifyFunction(
		pattern,
		17,
		"MWR::Null12",
		true,
		true)) return false;
	pattern.Update("48 8D 8F 90 00 00 00 E8 ?? ?? ?? ?? 48 89");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null13",
		true,
		true)) return false;
	pattern.Update("A0 00 00 00 41 C1 E0 05 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null14",
		true,
		true)) return false;
	pattern.Update("A8 00 00 00 41 C1 E1 05 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null15",
		true,
		true)) return false;
	pattern.Update("98 00 00 00 41 C1 E0 02 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null16",
		true,
		true)) return false;
	pattern.Update("98 00 00 00 41 C1 E0 02 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null17",
		true,
		true)) return false;
	pattern.Update("44 8D 04 40 45 03 C0 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null18",
		true,
		true)) return false;
	pattern.Update("44 8B 02 48 8B 52 08 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null19",
		true,
		true)) return false;
	pattern.Update("48 83 C2 10 48 8B CB E8 ?? ?? ?? ?? 48 83 C6 20");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null20",
		true,
		true)) return false;
	pattern.Update("E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 49 8B D6 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		6,
		"MWR::Null21",
		true,
		true)) return false;
	pattern.Update("0F 29 4C 24 70 E8 ?? ?? ?? ?? 48 8D 4C 24 20 E8");
	if (!Module.NullifyFunction(
		pattern,
		6,
		"MWR::Null22",
		true,
		true)) return false;
	pattern.Update("48 8B CB E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		4,
		"MWR::Null23",
		true,
		true)) return false;
	pattern.Update("48 8B CF E8 ?? ?? ?? ?? 48 89 1D ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		4,
		"MWR::Null24",
		true,
		true)) return false;
	pattern.Update("4D 8B 00 E8 ?? ?? ?? ?? 48 89 3D ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		4,
		"MWR::Null25",
		true,
		true)) return false;
	pattern.Update("48 83 C1 10 E8 ?? ?? ?? ?? 48 83 C4 28 C3");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null26",
		true,
		true)) return false;
	pattern.Update("49 8B D6 48 8B CE E8 ?? ?? ?? ?? E8");
	if (!Module.NullifyFunction(
		pattern,
		7,
		"MWR::Null27",
		true,
		true)) return false;
	pattern.Update("EB 05 E8 ?? ?? ?? 00 48 8B CF E8 ?? ?? ?? 00 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		11,
		"MWR::Null28",
		true,
		true)) return false;
	pattern.Update("B9 05 00 00 00 0F 11 05 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 48 08 48 89 0B E8");
	if (!Module.NullifyFunction(
		pattern,
		25,
		"MWR::Null29",
		true,
		true)) return false;
	pattern.Update("41 B8 01 00 00 00 48 8B 10 41 8D 48 10 48 8B 12 E8 ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		17,
		"MWR::Null29",
		true,
		true)) return false;

	*MW2CR_GraphicsDvar = MW2CR_MW2CR_GraphicsDvarBuffer;

	GameDirectory = gameDirectory;

	XAssetPoolCount      = 256;                                                         
	XAssetPools          = std::make_unique<XAssetPool[]>(XAssetPoolCount);
	Strings              = std::make_unique<char[]>(0x2000000);                         
	StringPoolSize       = 0;                                                           
	Initialized          = true;                                                        
	StringLookupTable    = std::make_unique<std::map<uint64_t, size_t>>();
	MW2CR_FastFileHandle = INVALID_HANDLE_VALUE;                                        

	Module.SaveCache("Data\\Dumps\\MW2CR_dump.cache");

// #if _DEBUG
// 	LoadAliases("F:\\Data\\Dumps\\VisualStudio\\Projects\\HydraX\\src\\HydraX\\bin\\x64\\Debug\\exported_files\\ModernWarfareRemasteredAliases.json");
// #else
// 	LoadAliases("Data\\Dumps\\ModernWarfareRemasteredAliases.json");
// #endif

	// LoadAliases(CurrentConfig->AliasesName);

	return true;
}

bool ps::CoDMW2RHandler::Deinitialize()
{
	MW2CR_DB_Reset();

	Module.Free();                    
	XAssetPoolCount        = 256;     
	XAssetPools            = nullptr;
	Strings                = nullptr;
	StringPoolSize         = 0;       
	Initialized            = false;   
	StringLookupTable      = nullptr;
	FileSystem             = nullptr;
	GameDirectory.clear();

	for (auto& MW2CR_SoundFile : MW2CR_SoundFiles)
	{
		if (MW2CR_SoundFile != nullptr && MW2CR_SoundFile != INVALID_HANDLE_VALUE)
		{
			CascCloseFile(MW2CR_SoundFile);
			MW2CR_SoundFile = nullptr;
		}
	}

	for (auto& MW2CR_LocalizedSoundFile : MW2CR_LocalizedSoundFiles)
	{
		if (MW2CR_LocalizedSoundFile != nullptr && MW2CR_LocalizedSoundFile != INVALID_HANDLE_VALUE)
		{
			CascCloseFile(MW2CR_LocalizedSoundFile);
			MW2CR_LocalizedSoundFile = nullptr;
		}
	}

	CascCloseStorage(MW2CR_StorageHandle);

	return true;
}

bool ps::CoDMW2RHandler::IsValid(const std::string& param)
{
	return strcmp(param.c_str(), "mw2cr") == 0;
}

// TODO:
// bool ps::CoDMW2RHandler::ListFiles()
// {
// 	std::vector<std::string> files;
// 	GetFiles("*", files);
// 	ps::log::Log(ps::LogType::Normal, "Listing files from: %s", GetName().c_str());
//
// 	for (auto& file : files)
// 	{
// 		ps::log::Log(ps::LogType::Normal, "File: %s Available: 1", file.c_str());
// 	}
//
// 	ps::log::Log(ps::LogType::Normal, "Listed: %lu files.", files.size());
//
// 	return true;
// }

bool ps::CoDMW2RHandler::DoesFastFileExists(const std::string& ffName)
{
	if (!Initialized)
	{
		return false;
	}

	auto fullName = ffName + ".ff";

	CASC_FIND_DATA findData{};
	HANDLE fileHandle = CascFindFirstFile(MW2CR_StorageHandle, fullName.c_str(), &findData, NULL);
	bool result = fileHandle != INVALID_HANDLE_VALUE && findData.bFileAvailable > 0;
	CascFindClose(fileHandle);
	return result;
}

bool ps::CoDMW2RHandler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
{
	ps::log::Log(ps::LogType::Normal, "Attempting to load: %s using handler: Call of Duty: Modern Warfare", ffName.c_str());

	if (!Initialized)
	{
		ps::log::Log(ps::LogType::Error, "Call of Duty: Modern Warfare 2 Campaign Remastered is not initialized, did you forget to call \"init\" with a game directory?");
		return false;
	}

	auto newFastFile = CreateUniqueFastFile(ffName);

	// An already loaded ff isn't a failure, so just report success
	if (newFastFile == nullptr)
	{
		ps::log::Log(ps::LogType::Normal, "The file: %s is already loaded.", ffName.c_str());
		return true;
	}

	newFastFile->Parent = parent;
	newFastFile->Flags = flags;

	// Set current ff for loading purposes.
	ps::Parasyte::PushTelemtry("LastFastFileName", ffName);

	bool success = true;

	MW2CR_DB_Reset();

	auto fastFileName = ffName + ".ff";

	MW2CR_CurrentFastFile = newFastFile;

	// Attempt to open main fast file, hard exit if we can't 
	// open the fast file.
	if (!CascOpenFile(MW2CR_StorageHandle, fastFileName.c_str(), CASC_LOCALE_NONE, CASC_OPEN_BY_NAME, &MW2CR_FastFileHandle) || !MW2CR_IsCascFileValid(MW2CR_FastFileHandle))
	{
		ps::log::Log(ps::LogType::Error, "The provided fast file: %s, could not be found in the CASC storage. Make sure any updates are finished and try launching your game to fix up the CASC storage.", ffName.c_str());
		return false;
	}

	MW2CR_XZoneMemory memory{};
	char required[256]{};
	MW2CR_DB_CalculateFileSize();
	MW2CR_DB_LoadXFile(&memory, nullptr, &required, false);
	MW2CR_DB_Reset();

	ps::log::Log(ps::LogType::Normal, "Successfully loaded: %s", ffName.c_str());

	// We must fix up any XModel surfs, as we may have overrode previous
	// temporary entries, etc.
	ps::Parasyte::GetCurrentHandler()->XAssetPools[7].EnumerateEntries([](ps::XAsset* asset)
	{
		MW2CR_DB_XModelSurfsFixup((__int64)asset->Header);
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

bool ps::CoDMW2RHandler::CleanUp()
{
	MW2CR_DB_Reset();
	return false;
}

PS_CINIT(ps::GameHandler, ps::CoDMW2RHandler, ps::GameHandler::GetHandlers());
#endif