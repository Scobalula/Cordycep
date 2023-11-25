#include "pch.h"
#include "UnloadAllCommand.h"

ps::UnloadAllCommand::UnloadAllCommand() : Command(
	"unloadall",
	"Unloads all files, including common files.",
	"This command unloads all files including those loaded by loadcommonfiles.",
	"unloadall",
	true)
{
}

void ps::UnloadAllCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	ps::log::Print("MAIN", "Unloading all files...");
	ps::Parasyte::GetCurrentHandler()->UnloadAllFastFiles();
	ps::log::Print("MAIN", "Successfully unloaded all files.");
}

PS_CINIT(ps::Command, ps::UnloadAllCommand, ps::Command::GetCommands());