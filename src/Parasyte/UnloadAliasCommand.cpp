#include "pch.h"
#include "UnloadAliasCommand.h"

ps::UnloadAliasCommand::UnloadAliasCommand() : Command(
	"unloadalias",
	"Unloads all files potentially loaded by the given alias.",
	"This command unloads all files that are associated with a given alias.",
	"unloadalias \"AK-47\" unloadalias \"Polina\" unloadalias \"Eagle's Nest\"",
	true)
{
}

void ps::UnloadAliasCommand::Execute(ps::CommandParser& parser) const
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
		ps::log::Print("MAIN", "Unloading %llu files, please wait...", count);

		size_t idx = 1;
		size_t unloadedFilesCount = 0;

		for (const auto& fileName : files)
		{
			ps::log::Print("MAIN", "Unloading %s...", fileName.c_str());
			const bool isUnloaded = ps::Parasyte::GetCurrentHandler()->UnloadFastFile(fileName);
			idx++;

			if (isUnloaded)
			{
				unloadedFilesCount++;
				ps::log::Print("MAIN", "Successfully unloaded: %s and all its assets.", fileName.c_str());
			}
		}

		ps::log::Print("MAIN", "Unloaded (%lu/%lu) files successfully.", unloadedFilesCount, count);
	}
}

PS_CINIT(ps::Command, ps::UnloadAliasCommand, ps::Command::GetCommands());