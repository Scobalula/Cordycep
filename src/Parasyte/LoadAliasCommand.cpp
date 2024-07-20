#include "pch.h"
#include "LoadAliasCommand.h"

ps::LoadAliasCommand::LoadAliasCommand() : Command(
	"loadalias",
	"Loads files from the alias cache with the given name.",
	"Loads files that are associated with the given alias. This allows you to load based off generated lists of names. Check the JSON files in the data folder for Aliases.",
	"loadalias \"AK-47\" loadalias \"Polina\" loadalias \"Eagle's Nest\"",
	true)
{
}

void ps::LoadAliasCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	std::string aliasName = parser.Next();
	std::ranges::transform(aliasName, aliasName.begin(), toupper);

	auto& aliases = ps::Parasyte::GetCurrentHandler()->Aliases;
	const auto range = aliases.equal_range(aliasName);

	if (range.first == range.second)
	{
		ps::log::Print("ERROR", "Cordycep couldn't find an alias matching that name, check the JSON file in the Data folder for valid aliases.");
		return;
	}

	for (auto it = range.first; it != range.second; ++it)
	{
		const auto& files = it->second.Files;
		const size_t count = files.size();

		ps::log::Print("MAIN", "Found %llu files.", count);
		ps::log::Print("MAIN", "Loading %llu files, please wait...", count);

		size_t idx = 1;
		size_t loadedFilesCount = 0;

		for (const auto& fileName : files)
		{
			const bool isLoaded = ps::Parasyte::LoadFile(fileName, idx, count, FastFileFlags::None);
			idx++;

			if (isLoaded)
				loadedFilesCount++;
		}

		ps::log::Print("MAIN", "Loaded (%lu/%lu) files successfully.", loadedFilesCount, count);
	}
}

PS_CINIT(ps::Command, ps::LoadAliasCommand, ps::Command::GetCommands());
