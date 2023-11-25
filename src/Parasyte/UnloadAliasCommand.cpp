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
	std::transform(aliasName.begin(), aliasName.end(), aliasName.begin(), toupper);

	auto alias = ps::Parasyte::GetCurrentHandler()->Aliases.find(aliasName);
	if (alias == ps::Parasyte::GetCurrentHandler()->Aliases.end())
	{
		ps::log::Print("ERROR", "Cordycep couldn't find an alias matching that name, check the JSON file in the Data folder for valid aliases.");
		return;
	}

	auto& files = alias->second.Files;
	ps::log::Print("MAIN", "Found %llu files.", files.size());
	ps::log::Print("MAIN", "Unloading %llu files, please wait...", files.size());

	size_t idx = 1;
	size_t count = files.size();

	for (auto& fileName : files)
	{
		ps::log::Print("MAIN", "Unloading %s...", fileName.c_str());
		ps::Parasyte::GetCurrentHandler()->UnloadFastFile(fileName);
		ps::log::Print("MAIN", "Successfully unloaded: %s and all its assets.", fileName.c_str());
	}

	ps::log::Print("MAIN", "Unloaded %llu files.", files.size());
}

PS_CINIT(ps::Command, ps::UnloadAliasCommand, ps::Command::GetCommands());