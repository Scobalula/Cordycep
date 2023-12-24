#include "pch.h"
#include "Parasyte.h"
#include "Oodle.h"

HMODULE Oodle = NULL;

int64_t(__fastcall* OodleLZ_Decompress)(
	uint8_t* compBuf, uint32_t compSize, uint8_t* rawBuf, uint32_t rawSize,
	int fuzzSafe,
	int checkCrc,
	int verbosity,
	uint8_t* dictBuff,
	uint64_t dictSize,
	uint64_t unk,
	void* unkCallback,
	uint8_t* scratchBuff,
	uint64_t scratchSize,
	int threadPhase);

bool ps::oodle::Initialize(const std::string& fileName)
{
	FreeLibrary(Oodle);
	Oodle = LoadLibraryA(fileName.c_str());

	if (Oodle == NULL)
	{
		ps::log::Log(ps::LogType::Error, "Failed to load Oodle's Module, ensure the game path you provided is correct and the platform is not in the middle of an update.");
		return false;
	};

	FARPROC proc = GetProcAddress((HMODULE)Oodle, "OodleLZ_Decompress");

	if (proc == NULL)
	{
		ps::log::Log(ps::LogType::Error, "Failed to find OodleLZ_Decompress, your Oodle Module is potentially corrupt.");
		return false;
	};

	OodleLZ_Decompress = reinterpret_cast<decltype(OodleLZ_Decompress)>(proc);
	return true;
}

size_t ps::oodle::Decompress(uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize)
{
	auto result = OodleLZ_Decompress(
		input,
		(uint32_t)inputSize,
		output,
		(uint32_t)outputSize,
		0,
		0,
		0,
		nullptr,
		0,
		0,
		nullptr,
		nullptr,
		0,
		3);

	return result;
}

bool ps::oodle::Clear()
{
	FreeLibrary(Oodle);
	return false;
}
