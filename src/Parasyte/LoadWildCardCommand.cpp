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

	const bool isFound = ps::Parasyte::GetCurrentHandler()->GetFiles(pattern, files) && !files.empty();

	if (!isFound)
	{
		ps::log::Print("ERROR", "Failed to find files with pattern.");
		return;
	}

	ps::log::Print("MAIN", "Found %llu files.", files.size());
	ps::log::Print("MAIN", "Loading %llu files, please wait...", files.size());

	size_t idx = 1;
	size_t loadedFilesCount = 0;
	size_t count = files.size();

	for (auto& fileName : files)
	{
		const bool isLoaded = ps::Parasyte::LoadFile(fileName, idx, count, FastFileFlags::None);
		idx++;

		if (isLoaded)
			loadedFilesCount++;
	}

	ps::log::Print("MAIN", "Loaded (%lu/%lu) files matching pattern successfully.", loadedFilesCount, files.size());
}

PS_CINIT(ps::Command, ps::LoadWildCardCommand, ps::Command::GetCommands());
