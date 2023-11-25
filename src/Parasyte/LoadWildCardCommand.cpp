#include "pch.h"
#include "LoadWildCardCommand.h"

ps::LoadWildCardCommand::LoadWildCardCommand() : Command(
	"loadwc",
	"Loads a file using wildcard matching if supported by the handler.",
	"This loads files using wildcard matching. Not supported by all handlers.",
	"loadwc *mpapa5*",
	true)
{

}

void ps::LoadWildCardCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	auto& pattern = parser.Next();
	std::vector<std::string> files;

	ps::log::Print("MAIN", "Searching for files with pattern, please wait...");

	if (ps::Parasyte::GetCurrentHandler()->GetFiles(pattern, files) && files.size() > 0)
	{
		if (files.size() > 0)
		{
			ps::log::Print("MAIN", "Found %llu files.", files.size());
			ps::log::Print("MAIN", "Loading %llu files, please wait...", files.size());

			size_t idx = 1;
			size_t count = files.size();

			for (auto& fileName : files)
			{
				ps::Parasyte::LoadFile(fileName, idx, count, FastFileFlags::None);
				idx++;
			}

			ps::log::Print("MAIN", "Loaded %llu files matching pattern.", files.size());
		}
		else
		{
			ps::log::Print("ERROR", "Failed to find files with pattern.");
		}
	}
	else
	{
		ps::log::Print("ERROR", "Failed to find files with pattern.");
	}
}

PS_CINIT(ps::Command, ps::LoadWildCardCommand, ps::Command::GetCommands());