#include "pch.h"
#include "UnloadAllNonCommonCommand.h"

ps::UnloadAllNonCommonCommand::UnloadAllNonCommonCommand() : Command(
	"unloadallnoncommon",
	"Unloads all non-common files.",
	"This command unloads all files except those loaded by loadcommonfiles.",
	"unloadallnoncommon",
	true)
{
}

void ps::UnloadAllNonCommonCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	ps::log::Print("MAIN", "Unloading all non-common files...");
	ps::Parasyte::GetCurrentHandler()->UnloadNonCommonFastFiles();
	ps::log::Print("MAIN", "Successfully unloaded all non-common files.");
}

PS_CINIT(ps::Command, ps::UnloadAllNonCommonCommand, ps::Command::GetCommands());