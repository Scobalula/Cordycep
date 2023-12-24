#include "pch.h"
#if _WIN64
#include "Parasyte.h"
#include "CoDGhostsHandler.h"
#include "lz4.h"
#include "ZLIBDecompressorV1.h"

namespace ps::CoDGhostsInternal
{
	// The fast file decompressor.
	std::unique_ptr<Decompressor> FFDecompressor;
	// The patch file decompressor.
	std::unique_ptr<Decompressor> FPDecompressor;
}

const std::string ps::CoDGhostsHandler::GetShorthand()
{
	return "ghosts";
}

const std::string ps::CoDGhostsHandler::GetName()
{
	return "Call of Duty: Ghosts (Indev)";
}

bool ps::CoDGhostsHandler::Initialize(const std::string& gameDirectory)
{
	Configs.clear();
	GameDirectory = gameDirectory;

	//LoadConfigs("Data\\Configs\\CoDMW4Handler.json");
	//SetConfig();
	//CopyDependencies();
	OpenGameDirectory(GameDirectory);
	//OpenGameModule(CurrentConfig->ModuleName);

	Initialized = true;
	return true;
}

bool ps::CoDGhostsHandler::Deinitialize()
{
	return true;
}

bool ps::CoDGhostsHandler::IsValid(const std::string& param)
{
	return param == "ghosts";
}

bool ps::CoDGhostsHandler::LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags)
{
	auto newFastFile = CreateUniqueFastFile(ffName);

	newFastFile->Parent = parent;
	newFastFile->Flags = flags;

	// Set current ff for loading purposes.
	ps::Parasyte::PushTelemtry("LastFastFileName", ffName);
	ps::Parasyte::SetCurrentFastFile(newFastFile);

	ps::FileHandle ffHandle(FileSystem->OpenFile(GetFileName(ffName + ".ff"), "r"), FileSystem.get());
	ps::FileHandle fpHandle(FileSystem->OpenFile(GetFileName(ffName + ".fp"), "r"), FileSystem.get());

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

	auto flag0 = ffHandle.Read<uint8_t>();
	auto flag1 = ffHandle.Read<uint8_t>();
	auto flag2 = ffHandle.Read<uint8_t>();
	auto flag3 = ffHandle.Read<uint8_t>();
	auto data0 = ffHandle.Read<uint32_t>();
	auto data1 = ffHandle.Read<uint32_t>();

	auto imageCount = ffHandle.Read<uint32_t>();
	auto imageData = ffHandle.ReadArray<uint8_t>((size_t)imageCount * 24);

	auto totalSize = ffHandle.Read<uint64_t>();
	auto authSize = ffHandle.Read<uint64_t>();

	ps::CoDGhostsInternal::FFDecompressor = std::make_unique<ZLIBDecompressorV1>(ffHandle, magic == 0x3030313066665749);

	char x[16384]{};

	std::ofstream out("test.dat", std::ios::binary);

	while (true)
	{
		auto size = ps::CoDGhostsInternal::FFDecompressor->Read(x, sizeof(x), 0);

		if (size == 0)
		{
			break;
		}

		out.write((const char*)&x, size);
	}

	return true;
}

bool ps::CoDGhostsHandler::CleanUp()
{
	return true;
}

// PS_CINIT(ps::GameHandler, ps::CoDGhostsHandler, ps::GameHandler::GetHandlers());
#endif