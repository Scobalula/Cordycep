#include "pch.h"
#include "MoveDirCommand.h"
#include "FileHandle.h"

ps::MoveDirCommand::MoveDirCommand() : Command(
	"movedir",
	"Moves the game's directory to another.",
	"This command handles moving the files Cordycep loads to another directory. Used for development/simulating file systems.",
	"movedir C:\\dir1 C:\\dir2",
	false)
{
}

void ps::MoveDirCommand::Execute(ps::CommandParser& parser) const
{
	auto& src = parser.Next();
	auto& dst = parser.Next();

	auto fs = ps::FileSystem::Open(src);

	ps::log::Print("MOVE", "Moving: %s to: %s...", src.c_str(), dst.c_str());

	auto dstPath = std::filesystem::path(dst);

	fs->EnumerateFiles("", "*.*", false, [&fs, dstPath](const std::string& path, const size_t size)
	{
		try
		{
			ps::log::Print("MOVE", "Moving file: %s...", path.c_str());

			auto handle = ps::FileHandle(fs->OpenFile(path, "r"), fs.get());
			auto outPath = dstPath / path;
			auto outDir = outPath.parent_path();

			std::filesystem::create_directories(outDir);

			std::ofstream s(outPath, std::ios::binary);

			uint8_t buf[8192]{};

			while (true)
			{
				auto consumed = handle.Read(buf, sizeof(buf));

				if (consumed == 0)
					break;

				s.write((const char*)buf, sizeof(buf));
			}

			ps::log::Print("MOVE", "Moved: %s successfully.", path.c_str());
		}
		catch (const std::exception& ex)
		{
			ps::log::Print("ERROR", "Failed to move: %s. Error: %s", path.c_str(), ex.what());
		}
	});
}

PS_CINIT(ps::Command, ps::MoveDirCommand, ps::Command::GetCommands());