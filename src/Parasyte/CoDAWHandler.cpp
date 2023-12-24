#include "pch.h"
#if _WIN64
#include "Parasyte.h"
#include "CoDAWHandler.h"
#include "lz4.h"
#include <intrin.h>
#include <array>

// The fast file file handle.
HANDLE AW_FastFileHandle = NULL;
// Whether or not the current file is secure.
int AW_FileIsSecure = 0;
// Current compressor
int AW_FileCompression = -1;
// Current Fast File 
ps::FastFile* AW_CurrentFastFile = nullptr;
// Sound Files
std::array<HANDLE, 512> AW_SoundFiles;

const char* (__fastcall* AW_DB_GetXAssetName)(void* xassetHeader);

const char* (__fastcall* AW_DB_SetXAssetName)(void* xassetHeader, const char* name);

const size_t (__fastcall* AW_DB_GetXAssetTypeSizeInternal)(uint32_t xassetType);

struct AW_XImageData
{
	uint16_t Locale;
	uint16_t PackageIndex;
	uint32_t Checksum;
	uint64_t Offset;
	uint64_t Size;
};

// Blank Graphics Dvar Buffer
char AW_AW_GraphicsDvarBuffer[512]{};
// Graphics Related Dvar
char** AW_GraphicsDvar = nullptr;
// Images Count
uint32_t* AW_ImageCount = nullptr;
// Current Images Index
uint32_t AW_ImageIndex = 0;
// Images Buffer
AW_XImageData* AW_ImagesBuffer = nullptr;

// Zone Memory Block
struct AW_XBlock
{
	void* data;
	size_t size;
};

// Zone Memory
struct AW_XZoneMemory
{
	AW_XBlock blocks[7];
};

// Requests sound file data for loaded audio.
void AW_DB_RequestSoundFileData(void* ptr, uint16_t index, size_t fileOffset, size_t size, const char* name)
{
	if (!ptr)
		return;
	if (fileOffset == 0 || size == 0)
		return;

	HANDLE handle = AW_SoundFiles[index];

	if (handle == NULL || handle == INVALID_HANDLE_VALUE)
	{
		auto soundFilePath = ps::Parasyte::GetCurrentHandler()->GameDirectory + "\\" + "soundfile" + std::to_string(index) + ".pak";
		AW_SoundFiles[index] = CreateFileA(
			soundFilePath.c_str(),
			FILE_READ_DATA | FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	}

	if (handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER Input{};
		Input.QuadPart = fileOffset;

		if (SetFilePointerEx(handle, Input, NULL, FILE_BEGIN))
		{
			if (!ReadFile(handle, ptr, (DWORD)size, NULL, NULL))
			{
				ps::log::Log(ps::LogType::Verbose, "WARNING: Failed to load sound data for sound: %s.", name);
			}
		}
	}
	else
	{
		ps::log::Log(ps::LogType::Verbose, "WARNING:  Failed to open sound file for sound: %s.", name);
	}
}

// Seeks into further into the file from where we are.
void AW_DB_Seek(size_t v)
{
	LARGE_INTEGER Input{};
	Input.QuadPart = v;

	if (!SetFilePointerEx(AW_FastFileHandle, Input, NULL, FILE_CURRENT))
	{
		throw std::exception("Failed to seek in fast file.");
	}
}

// Seeks into further into the file from where we are.
void AW_DB_SetPosition(size_t v)
{
	LARGE_INTEGER Input{};
	Input.QuadPart = v;

	if (!SetFilePointerEx(AW_FastFileHandle, Input, NULL, FILE_BEGIN))
	{
		throw std::exception("Failed to seek in fast file.");
	}
}

// Reads from the file.
void AW_DB_Read(void* pos, size_t size)
{
	DWORD bytesRead = 0;

	if (!ReadFile(AW_FastFileHandle, pos, (DWORD)size, &bytesRead, NULL))
	{
		throw std::exception("Failed to read data from the fast file.");
	}

	if (bytesRead != size)
	{
		throw std::exception("Failed to read from fast file.");
	}
}

// Gets the current position of the file.
size_t AW_DB_Position()
{
	LARGE_INTEGER Input{};

	if (!SetFilePointerEx(AW_FastFileHandle, { 0 }, &Input, FILE_CURRENT))
	{
		throw std::exception("Failed to request position from fast file.");
	}

	return Input.QuadPart;
}

// Distance to the next hash block
bool AW_AuthBlockConsumed = false;
// Distance to the next hash block
size_t AW_DistanceToNextHashBlock = 0;

// Reads uncompressed data from the file while accounting for hash blocks.
void AW_DB_ReadXFileUncompressed(void* pos, size_t size)
{
	if (!AW_FileIsSecure)
	{
		AW_DB_Read(pos, size);
	}
	else
	{
		if (!AW_AuthBlockConsumed)
		{
			AW_DB_Seek(0x8000);
			AW_DistanceToNextHashBlock = 0x800000;
			AW_AuthBlockConsumed = true;
		}

		char* ptrInternal = (char*)pos;
		size_t dataConsumed = 0;

		if (AW_DistanceToNextHashBlock < size)
		{
			AW_DB_Read(ptrInternal, AW_DistanceToNextHashBlock);
			AW_DB_Seek(0x4000);
			dataConsumed += AW_DistanceToNextHashBlock;
			ptrInternal += dataConsumed;
			AW_DistanceToNextHashBlock = 0x800000;
		}

		auto trail = size - dataConsumed;

		AW_DB_Read(ptrInternal, trail);
		AW_DistanceToNextHashBlock -= trail;
	}
}

// The current compressed buffer
std::unique_ptr<uint8_t[]> AW_CompressedBuffer;
// The current offset within the compressed buffer
uint32_t AW_CompressedBufferOffset = 0;
// The size of the compressed buffer
uint32_t AW_CompressedBufferSize = 0;
// The capacity of the compressed Buffer
uint32_t AW_CompressedBufferCapacity = 0;

// The current decompressed buffer
std::unique_ptr<uint8_t[]> AW_DecompressedBuffer;
// The current offset within the decompressed buffer
uint32_t AW_DecompressedBufferOffset = 0;
// The size of the decompressed buffer
uint32_t AW_DecompressedBufferSize = 0;
// The capacity of the decompressed Buffer
uint32_t AW_DecompressedBufferCapacity = 0;
// The current fast file bloc.
uint32_t AW_CurrentBlock = 0;

// The current position in the raw file data.
size_t AW_CurrentRawFilePosition = 0;
// The size of the compressed file.
size_t AW_CompressedFileSize = 0;
// The current position in the compressed file.
size_t AW_CompressedFilePosition = 0;

void AW_DB_AllocFastFileHeaders()
{

}

// Resets all data.
void AW_DB_Reset()
{
	AW_ImageIndex                  = 0;
	AW_CurrentBlock				= 0;
	AW_DecompressedBuffer			= nullptr;
	AW_DecompressedBufferOffset	= 0;
	AW_DecompressedBufferSize		= 0;
	AW_DecompressedBufferCapacity	= 0;
	AW_CompressedBuffer			= nullptr;
	AW_CompressedBufferOffset	    = 0;
	AW_CompressedBufferSize		= 0;
	AW_CompressedBufferCapacity	= 0;
	AW_CurrentRawFilePosition		= 0;
	AW_CurrentFastFile				= nullptr;
	AW_DistanceToNextHashBlock		= 0;
	AW_FileIsSecure                = 0;
	AW_CompressedFilePosition      = 0;
	AW_CompressedFileSize			= 0;
	AW_AuthBlockConsumed = false;

	CloseHandle(AW_FastFileHandle);
	AW_FastFileHandle = INVALID_HANDLE_VALUE;
}

void AW_DB_ReadXFile(void* ptr, size_t size)
{
	char* ptrInternal = (char*)ptr;
	size_t remaining = size;
	AW_CurrentRawFilePosition += size;

	while (true)
	{
		auto cacheRemaining = AW_DecompressedBufferSize - AW_DecompressedBufferOffset;

		if (cacheRemaining > 0)
		{
			auto toCopy = min(cacheRemaining, remaining);
			std::memcpy(ptrInternal, &AW_DecompressedBuffer[AW_DecompressedBufferOffset], toCopy);

			ptrInternal += toCopy;
			AW_DecompressedBufferOffset += (uint32_t)toCopy;
			remaining -= toCopy;
		}

		if (remaining <= 0)
			break;

		if (AW_CurrentBlock == 0)
		{
			char info[12]{};
			AW_DB_ReadXFileUncompressed(info, sizeof(info));
		}

		AW_DB_ReadXFileUncompressed(&AW_CompressedBufferSize, sizeof(AW_CompressedBufferSize));
		AW_DB_ReadXFileUncompressed(&AW_DecompressedBufferSize, sizeof(AW_DecompressedBufferSize));

		// Realloc if required
		if (AW_CompressedBufferSize > AW_CompressedBufferCapacity)
		{
			AW_CompressedBufferCapacity = ((size_t)AW_CompressedBufferSize + 4095) & 0xFFFFFFFFFFFFF000;
			if (AW_CompressedBufferCapacity < 0x10000)
				AW_CompressedBufferCapacity = 0x10000;
			AW_CompressedBuffer = std::make_unique<uint8_t[]>(AW_CompressedBufferCapacity);
		}
		if (AW_DecompressedBufferSize > AW_DecompressedBufferCapacity)
		{
			AW_DecompressedBufferCapacity = ((size_t)AW_DecompressedBufferSize + 4095) & 0xFFFFFFFFFFFFF000;
			if (AW_DecompressedBufferCapacity < 0x10000)
				AW_DecompressedBufferCapacity = 0x10000;
			AW_DecompressedBuffer = std::make_unique<uint8_t[]>(AW_DecompressedBufferCapacity);
		}

		AW_CompressedBufferOffset = 0;
		AW_DecompressedBufferOffset = 0;

		AW_DB_ReadXFileUncompressed(&AW_CompressedBuffer[0], ((size_t)AW_CompressedBufferSize + 0x3) & 0xFFFFFFFFFFFFFFC);

		auto result = LZ4_decompress_safe(
			(const char*)AW_CompressedBuffer.get(),
			(char*)AW_DecompressedBuffer.get(),
			AW_CompressedBufferSize,
			AW_DecompressedBufferSize);

		if (result != AW_DecompressedBufferSize)
		{
			ps::log::Log(ps::LogType::Error, "Failed to decompress data.");
			throw new std::exception();
		}

		AW_CurrentBlock++;
	}
}

void AW_DB_AllocXZoneMemory(unsigned __int64* blockSize, const char* filename, AW_XZoneMemory* zoneMem)
{
	for (int i = 0; i < 7; i++)
	{
		AW_CurrentFastFile->MemoryBlocks[i].Initialize(blockSize[i], 0x1000);

		zoneMem->blocks[i].size = AW_CurrentFastFile->MemoryBlocks[i].MemorySize;
		zoneMem->blocks[i].data = AW_CurrentFastFile->MemoryBlocks[i].Memory;
	}
}

void AW_DB_SetCompressor(int compression)
{
	AW_FileCompression = compression;
}

void AW_DB_InflateInit(int fileIsSecure)
{
	AW_FileIsSecure = fileIsSecure;
}

void AW_DB_CalculateFileSize()
{
	LARGE_INTEGER Output{};

	if (!SetFilePointerEx(AW_FastFileHandle, { 0 }, &Output, FILE_END))
	{
		throw std::exception("Failed to seek in fast file.");
	}

	AW_CompressedFileSize = Output.QuadPart;
	AW_DB_SetPosition(0);
}

typedef uint32_t AW_XAssetType;

struct AW_XAssetHeader
{
	void* Header;
};

struct AW_XAsset
{
	AW_XAssetType type;
	AW_XAssetHeader header;
};

struct AW_XAssetEntry
{
	AW_XAsset asset;
	char zoneIndex;
	volatile char inuseMask;
	unsigned int nextHash;
	unsigned int nextOverride;
	unsigned int nextPoolEntry;
};

AW_XAssetEntry AW_DB_LinkedXAsset{};

size_t AW_DB_GetXAssetTypeSize(AW_XAssetType xassetType)
{
	auto result = AW_DB_GetXAssetTypeSizeInternal(xassetType);

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

AW_XAssetEntry* DB_LinkAW_XAssetEntry(AW_XAssetType xassetType, AW_XAssetHeader* asset)
{
	AW_XAsset xasset
	{
		xassetType,
		*asset
	};

	auto name = AW_DB_GetXAssetName(&xasset);
	auto pool = &ps::Parasyte::GetCurrentHandler()->XAssetPools[xassetType];
	auto temp = name[0] == ',';
	auto size = AW_DB_GetXAssetTypeSize(xassetType);

	// Ensure pool is initialized
	pool->Initialize(size, 16384);

	if (temp)
	{
		AW_DB_SetXAssetName(&xasset, name + 1);
		name = AW_DB_GetXAssetName(&xasset);
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
				AW_CurrentFastFile,
				(uint8_t*)asset->Header,
				temp);
		}
		else
		{
			result->Override(
				AW_CurrentFastFile,
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
			AW_CurrentFastFile,
			(uint8_t*)asset->Header,
			temp);
	}

	// Loggary for Stiggary
	ps::log::Log(ps::LogType::Verbose, "%s Type: %i @ %llu @ %llu", name, xassetType, (uint64_t)result->Header, (uint64_t)result->Temp);

	AW_DB_LinkedXAsset.asset.type = xassetType;
	AW_DB_LinkedXAsset.asset.header.Header = result->Header;

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
				if (*AW_ImageCount == 0)
				{
					auto xasset = result->FindChildOfFastFile(AW_CurrentFastFile->Parent);

					if (xasset != nullptr)
					{
						std::memcpy(header + 104 + sizeof(AW_XImageData), xasset->Header + 104 + sizeof(AW_XImageData), sizeof(AW_XImageData) * 4);
					}
				}
				else
				{
					for (size_t i = 0; i < 4; i++)
					{
						// Should never happen on a valid Fast File
						if (AW_ImageIndex >= *AW_ImageCount)
							throw std::exception("Invalid image index, corrupt PAK table in fast file.");

						auto image = AW_ImagesBuffer[AW_ImageIndex++];
						// Copy to end of the header so that tools can read directly from it
						std::memcpy(header + 104 + sizeof(AW_XImageData) * i, &image, sizeof(AW_XImageData));
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
			AW_DB_RequestSoundFileData(
				*(void**)(header + 32),
				*(uint16_t*)(header + 10),
				*(uint64_t*)(header + 16),
				*(uint32_t*)(header + 24),
				name);
			break;
		}
		}
	}

	return &AW_DB_LinkedXAsset;
}

void* DB_FindAW_XAssetHeader(AW_XAssetType xassetType, const char* name, int allowCreateDefault)
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
			(uint32_t)AW_DB_GetXAssetTypeSize(xassetType),
			AW_CurrentFastFile,
			nullptr,
			true);

		AW_XAsset asset
		{
			xassetType,
			result->Header
		};
		AW_DB_SetXAssetName(&asset, name);

		return result->Header;
	}

	return nullptr;
}

__int64 AW_DB_GetString(const char* str, unsigned int user)
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

void* (__cdecl* AW_DB_LoadXFile)(const void* a, const void* b, const void* c, const bool d);

const std::string ps::CoDAWHandler::GetShorthand()
{
	return "aw";
}

const std::string ps::CoDAWHandler::GetName()
{
	return "Call of Duty: Advanced Warfare";
}

bool ps::CoDAWHandler::GetFiles(const std::string& pattern, std::vector<std::string>& results)
{
	HANDLE findHandle;
	WIN32_FIND_DATA findData;

	auto fullPath = GameDirectory + "/" + pattern;
	findHandle = FindFirstFileA(fullPath.c_str(), &findData);

	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string_view fileName = findData.cFileName;

			if (fileName.ends_with(".ff") &&
				fileName.find('\\') == std::string::npos &&
				fileName.find('/') == std::string::npos)
			{
				results.emplace_back(fileName.substr(0, fileName.find(".ff")));
			}
		} while (FindNextFileA(findHandle, &findData));

		FindClose(findHandle);
	}

	return true;
}

const std::vector<std::string>& ps::CoDAWHandler::GetCommonFileNames() const
{
	static std::vector<std::string> results
	{
		"code_post_gfx_mp",
		"code_pre_gfx_mp",
		"ui_mp",
		"common_mp",
		"mp_vlobby_room",
	};

	return results;
}

__int64 __fastcall AW_DB_XModelSurfsFixup(__int64 a1)
{
	__int16 v1; // r8
	__int64 result; // rax
	int v3; // er10
	uint64_t* v4; // rdx
	__int64 v5; // r9

	v1 = 0;
	result = 0xFFFDi64;
	*(uint16_t*)(a1 + 516) &= 0xFFFDu;
	v3 = 0;
	if (*(char*)(a1 + 512) > 0)
	{
		v4 = (uint64_t*)(a1 + 176);
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
			v4 += 8;
			result = (unsigned int)*(char*)(a1 + 512);
		} while (v3 < (int)result);
	}
	*(uint8_t*)(a1 + 10) = (uint8_t)v1;
	return result;
}

bool ps::CoDAWHandler::Initialize(const std::string& gameDirectory)
{
	auto& gameName = GetName();

	ps::log::Log(ps::LogType::Normal, "Attempting to initialize: %s...", gameName.c_str());
	ps::log::Log(ps::LogType::Normal, "Attempting to open directory: %s", gameDirectory.c_str());

	if (!std::filesystem::exists(gameDirectory))
	{
		ps::log::Log(ps::LogType::Error, "Failed to locate %s's directory at: %s", gameName.c_str(), gameDirectory.c_str());
		return false;
	}

	ps::log::Log(ps::LogType::Normal, "Successfully opened %'s directory: %s", gameName.c_str(), gameDirectory.c_str());

	if (!Module.Load("Data\\Dumps\\s1_mp64_ship_dump.exe"))
	{
		ps::log::Log(ps::LogType::Error, "Failed to load %s's Module: Data\\Dumps\\h1_mp64_ship_dump.exe", gameName.c_str());
		ps::log::Log(ps::LogType::Error, "Please refer to the Wiki for information producing a game dump.");
		return false;
	}

	Module.LoadCache("Data\\Dumps\\s1_mp64_ship_dump.cache");

	Pattern pattern;

	bool success = true;

	pattern.Update("48 8B D7 E8 ?? ?? ?? ?? 45 85 FF 74 33 0F");
	if (!Module.FindVariableAddress(
		&AW_DB_LoadXFile,
		pattern,
		4,
		"IW8::Var0",
		ps::ScanType::FromEndOfData)) success = false;
	pattern.Update("75 ?? 48 8B CB E8 1B 1D FD FF 48 8B D5 48 8B C8 E8");
	if (!Module.FindVariableAddress(
		&AW_DB_GetXAssetName,
		pattern,
		6,
		"IW8::Var1",
		ps::ScanType::FromEndOfData)) success = false;
	pattern.Update("48 8B D0 E8 ?? ?? ?? ?? 0F B6 45 12 A8 08 74");
	if (!Module.FindVariableAddress(
		&AW_DB_SetXAssetName,
		pattern,
		4,
		"IW8::Var2",
		ps::ScanType::FromEndOfData)) success = false;
	pattern.Update("B9 07 00 00 00 E8 ?? ?? ?? ?? 48 8B 4C 24 28 48");
	if (!Module.FindVariableAddress(
		&AW_DB_GetXAssetTypeSizeInternal,
		pattern,
		6,
		"IW8::Var3",
		ps::ScanType::FromEndOfData)) success = false;
	pattern.Update("74 36 48 8B 05 ?? ?? ?? ?? 80 78 10 00 74");
	if (!Module.FindVariableAddress(
		&AW_GraphicsDvar,
		pattern,
		5,
		"IW8::Var4",
		ps::ScanType::FromEndOfData)) success = false;
	pattern.Update("E8 ?? ?? ?? ?? 8B 05 ?? ?? ?? ?? 3D 80 D7 00 00");
	if (!Module.FindVariableAddress(
		&AW_ImageCount,
		pattern,
		7,
		"IW8::Var5",
		ps::ScanType::FromEndOfData)) success = false;
	pattern.Update("74 26 48 8D 35 ?? ?? ?? ?? 0F 1F 00 48 8D 0C 5B BA 18 00 00 00");
	if (!Module.FindVariableAddress(
		&AW_ImagesBuffer,
		pattern,
		5,
		"IW8::Var6",
		ps::ScanType::FromEndOfData)) success = false;

	// If we don't find any of these, it's a hard fail like Harry's modding career.
	// Variables
	pattern.Update("BA 04 00 00 00 E8 ?? ?? ?? ?? 48 8D 4D E7 BA 04 00 00 00");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		6,
		"MWR::Det0",
		ps::ScanType::FromEndOfData), (uintptr_t)&AW_DB_ReadXFileUncompressed)) success = false;
	pattern.Update("48 8D 4D F7 41 8D 50 48 E8 ?? ?? ?? ?? E8");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		9,
		"MWR::Det1",
		ps::ScanType::FromEndOfData), (uintptr_t)&AW_DB_ReadXFile)) success = false;
	pattern.Update("C7 05 5A 56 ?? ?? ?? ?? ?? 00 E8 ?? ?? ?? 00 45");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		11,
		"MWR::Det2",
		ps::ScanType::FromEndOfData), (uintptr_t)&AW_DB_InflateInit)) success = false;
	pattern.Update("0F B6 4D 7F E8 ?? ?? ?? 00 48 8D 0D ?? ?? ?? ?? BA 04");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		5,
		"MWR::Det3",
		ps::ScanType::FromEndOfData), (uintptr_t)&AW_DB_SetCompressor)) success = false;
	pattern.Update("4D 8B C6 48 8B D6 E8 ?? ?? ?? ?? 49 8B CE");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		7,
		"MWR::Det4",
		ps::ScanType::FromEndOfData), (uintptr_t)&AW_DB_AllocXZoneMemory)) success = false;
	pattern.Update("74 ?? BA 04 00 00 00 E8 ?? ?? ?? ?? 8B C8");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		8,
		"MWR::Det5",
		ps::ScanType::FromEndOfData), (uintptr_t)&AW_DB_GetString)) success = false;
	pattern.Update("B9 43 00 00 00 48 89 05 ?? ?? ?? ?? 48 8B 44 24 28 48 89 05 ?? ?? ?? ?? E8");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		25,
		"MWR::Det6",
		ps::ScanType::FromEndOfData), (uintptr_t)&DB_LinkAW_XAssetEntry)) success = false;
	pattern.Update("4A 8B 4C 37 08 E8 ?? ?? ?? ?? 42 8B 44 37 18");
	if (!Module.CreateDetour((uintptr_t)Module.FindVariableAddress(
		pattern,
		6,
		"MWR::Det7",
		ps::ScanType::FromEndOfData), (uintptr_t)&AW_DB_XModelSurfsFixup)) success = false;

	pattern.Update("CF 4D 8B F8 E8 ?? ?? ?? ?? 48 8D 4D CF BA 08 00");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 20 48 8B CB 44 8D 04 40 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		12,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("48 8D 4F 50 45 8B 48 28 4D 8B 40 38 33 D2 E8 ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		15,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("CB E8 ?? ?? ?? ?? 48 83 C5 28 48 89 3D ?? ?? ?? ?? 48 FF CE 0F");
	if (!Module.NullifyFunction(
		pattern,
		2,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("E8 ?? ?? ?? ?? 48 8B 3D ?? ?? ?? ?? 41 B8 08 00 00 00 48 81 C7 00 01 00 00");
	if (!Module.NullifyFunction(
		pattern,
		1,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("45 33 C0 48 8D 15 ?? ?? ?? ?? 41 8D 48 3B E8 ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		15,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 08 E8 ?? ?? ?? ?? 48 89 1D ?? ?? ?? ?? E8");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 60 48 8B CB 41 C1 E0 05 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		12,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 68 41 C1 E1 05 48 8B CB 48 89 44 24 20 E8 ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		17,
		"MWR::Null1",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 10 E8 ?? ?? ?? ?? 48 89 1D ?? ?? ?? ?? E8");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null2",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 18 44 0F AF C0 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null5",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 78 41 C1 E0 02 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		9,
		"MWR::Null9",
		true,
		true)) success = false;
	pattern.Update("41 C1 E0 02 E8 ?? ?? ?? ?? 48 89 3D ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null10",
		true,
		true)) success = false;
	pattern.Update("48 8D 8F 90 00 00 00 E8 ?? ?? ?? ?? 48 89");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null12",
		true,
		true)) success = false;
	pattern.Update("44 8D 04 40 45 03 C0 E8 ?? ?? ?? ?? 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null17",
		true,
		true)) success = false;
	pattern.Update("48 83 C2 10 48 8B CB E8 ?? ?? ?? ?? 48 83 C6 20");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null19",
		true,
		true)) success = false;
	pattern.Update("E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 49 8B D6 48 8B");
	if (!Module.NullifyFunction(
		pattern,
		6,
		"MWR::Null20",
		true,
		true)) success = false;
	pattern.Update("48 8B CB E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		4,
		"MWR::Null22",
		true,
		true)) success = false;
	pattern.Update("48 8B CB E8 ?? ?? ?? ?? 48 83 C6 40 48 89");
	if (!Module.NullifyFunction(
		pattern,
		4,
		"MWR::Null23",
		true,
		true)) success = false;
	pattern.Update("48 8B CF E8 ?? ?? ?? ?? 48 89 1D ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		4,
		"MWR::Null26",
		true,
		true)) success = false;
	pattern.Update("48 83 C1 10 E8 ?? ?? ?? ?? 48 83 C4 28 C3");
	if (!Module.NullifyFunction(
		pattern,
		5,
		"MWR::Null28",
		true,
		true)) success = false;
	pattern.Update("49 8B D6 48 8B CE E8 ?? ?? ?? ?? E8");
	if (!Module.NullifyFunction(
		pattern,
		7,
		"MWR::Null30",
		true,
		true)) success = false;
	pattern.Update("0F B6 50 08 4D 8B 00 48 8B CB E8 ?? ?? ?? ?? 48");
	if (!Module.NullifyFunction(
		pattern,
		11,
		"MWR::Null30",
		true,
		true)) success = false;
	pattern.Update("48 8B 52 08 48 8B CB E8 ?? ?? ?? ?? 48 8B 1D ?? ?? ?? ?? 48 89");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null30",
		true,
		true)) success = false;
	pattern.Update("48 83 C2 10 48 8B CB E8 ?? ?? ?? 00 48 83 C6 20 48 89 3D");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null30",
		true,
		true)) success = false;
	pattern.Update("48 8B 15 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 48 89 35 ?? ?? ?? ?? 48 8B 74 24 30 48");
	if (!Module.NullifyFunction(
		pattern,
		11,
		"MWR::Null30",
		true,
		true)) success = false;
	pattern.Update("E8 ?? ?? ?? ?? 48 8D 4C 24 50 E8 ?? ?? ?? ?? 0F 1F 44 00 00");
	if (!Module.NullifyFunction(
		pattern,
		11,
		"MWR::Null31",
		true,
		true)) success = false;
	pattern.Update("F0 FF 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 74 ?? C6 05 ?? ?? ?? ?? 01 0F 1F 80 00 00 00 00");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null32",
		true,
		true)) success = false;
	pattern.Update("F0 FF 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 74 ?? C6 05 ?? ?? ?? ?? 01 0F 1F 80 00 00 00 00");
	if (!Module.NullifyFunction(
		pattern,
		8,
		"MWR::Null32",
		true,
		true)) success = false;

	pattern.Update("48 83 C3 10 48 FF CF 75 E9 E8 ?? ?? ?? ?? 90 E9");
	if (!Module.PatchBytes(
		pattern,
		15,
		"MWR::Null30",
		(PBYTE)"\x48\x81\xc4\xd0\x00\x00\x00\x41\x5F\x41\x5E\x5D\x48\x8B\x7C\x24\x18\x48\x8b\x74\x24\x10\xC3",
		24,
		true,
		false)) success = false; // Patch obfuscated call, restore original values and return back.

	if (success)
	{
		*AW_GraphicsDvar = AW_AW_GraphicsDvarBuffer;

		GameDirectory = gameDirectory;

		XAssetPoolCount = 256;
		XAssetPools = std::make_unique<XAssetPool[]>(XAssetPoolCount);
		Strings = std::make_unique<char[]>(0x2000000);
		StringPoolSize = 0;
		Initialized = true;
		StringLookupTable = std::make_unique<std::map<uint64_t, size_t>>();
		AW_FastFileHandle = INVALID_HANDLE_VALUE;

		Module.SaveCache("Data\\Dumps\\s1_mp64_ship_dump.cache");

// #if _DEBUG
// 		LoadAliases("F:\\Data\\Dumps\\VisualStudio\\Projects\\HydraX\\src\\HydraX\\bin\\x64\\Debug\\exported_files\\ModernWarfareRemasteredAliases.json");
// #else
// 		LoadAliases("Data\\Dumps\\ModernWarfareRemasteredAliases.json");
// #endif
		// LoadAliases(CurrentConfig->AliasesName);
	}


	return success;
}

bool ps::CoDAWHandler::Deinitialize()
{
	AW_DB_Reset();

	Module.Free();
	XAssetPoolCount   = 256;
	XAssetPools       = nullptr;
	Strings           = nullptr;
	StringPoolSize    = 0;
	Initialized       = false;
	StringLookupTable = nullptr;
	FileSystem        = nullptr;
	GameDirectory.clear();

	// Clear out open handles to reference files.
	for (auto& AW_SoundFile : AW_SoundFiles)
	{
		if (AW_SoundFile != NULL && AW_SoundFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(AW_SoundFile);
			AW_SoundFile = NULL;
		}
	}

	return true;
}

bool ps::CoDAWHandler::IsValid(const std::string& param)
{
	return strcmp(param.c_str(), "aw") == 0;
}

bool ps::CoDAWHandler::DoesFastFileExists(const std::string& ffName)
{
	if (!Initialized)
	{
		return false;
	}

	return std::filesystem::exists(GameDirectory + "\\" + ffName + ".ff");
}

bool ps::CoDAWHandler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
{
	ps::log::Log(ps::LogType::Normal, "Attempting to load: %s using handler: %s", ffName.c_str(), GetName().c_str());

	if (!Initialized)
	{
		ps::log::Log(ps::LogType::Error, "%s is not initialized, did you forget to call \"init\" with a game directory?", GetName().c_str());
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

	AW_DB_Reset();

	auto fullPath = ps::Parasyte::GetCurrentHandler()->GameDirectory + "\\" + ffName + ".ff";

	AW_CurrentFastFile = newFastFile;
	AW_FastFileHandle = CreateFileA(
		fullPath.c_str(),
		FILE_READ_DATA | FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (AW_FastFileHandle == INVALID_HANDLE_VALUE)
	{
		ps::log::Log(ps::LogType::Error, "The provided fast file: %s, could not be found in the game's directory. Make sure any updates are finished and check for any content packages.", ffName.c_str());
		return false;
	}

	AW_XZoneMemory memory{};
	char required[256]{};
	AW_DB_CalculateFileSize();
	AW_DB_LoadXFile(&memory, nullptr, &required, false);
	AW_DB_Reset();

	ps::log::Log(ps::LogType::Normal, "Successfully loaded: %s", ffName.c_str());

	// We must fix up any XModel surfs, as we may have overrode previous
	// temporary entries, etc.
	ps::Parasyte::GetCurrentHandler()->XAssetPools[7].EnumerateEntries([](ps::XAsset* asset)
	{
		AW_DB_XModelSurfsFixup((__int64)asset->Header);
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
		if (RegionPrefix.size() > 0)
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

bool ps::CoDAWHandler::CleanUp()
{
	AW_DB_Reset();
	return false;
}

PS_CINIT(ps::GameHandler, ps::CoDAWHandler, ps::GameHandler::GetHandlers());
#endif