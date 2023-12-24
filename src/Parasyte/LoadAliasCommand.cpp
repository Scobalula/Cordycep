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

	const auto alias = ps::Parasyte::GetCurrentHandler()->Aliases.find(aliasName);
	if (alias == ps::Parasyte::GetCurrentHandler()->Aliases.end())
	{
		ps::log::Print("ERROR", "Cordycep couldn't find an alias matching that name, check the JSON file in the Data folder for valid aliases.");
		return;
	}

	const auto& files = alias->second.Files;
	ps::log::Print("MAIN", "Found %llu files.", files.size());
	ps::log::Print("MAIN", "Loading %llu files, please wait...", files.size());

	size_t idx = 1;
	size_t loadedFilesCount = 0;
	const size_t count = files.size();

	for (auto& fileName : files)
	{
		const bool isLoaded = ps::Parasyte::LoadFile(fileName, idx, count, FastFileFlags::None);
		idx++;

		if (isLoaded)
			loadedFilesCount++;
	}

	ps::log::Print("MAIN", "Loaded (%llu/%llu) files successfully.", loadedFilesCount, files.size());
}

PS_CINIT(ps::Command, ps::LoadAliasCommand, ps::Command::GetCommands());
