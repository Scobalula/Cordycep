#include "pch.h"
#include "DeinitCommand.h"

ps::DeinitCommand::DeinitCommand() : Command(
	"deinit",
	"Unloads all files and deinitializes the current handler.",
	"This command handles clearing all memory, files, and modules used by the handler, and essentially shuts it down.",
	"deinit",
	true)
{
}

void ps::DeinitCommand::Execute(ps::CommandParser& parser) const
{
	DeinitializeCurrentHandler(true);
}

void ps::DeinitCommand::DeinitializeCurrentHandler(bool implicit)
{
	if (ps::Parasyte::GetCurrentHandler() != nullptr)
	{
		ps::log::Print("MAIN", "Shutting down %s...", ps::Parasyte::GetCurrentHandler()->GetName().c_str());
		ps::log::Print("MAIN", "Unloading files...");
		ps::Parasyte::GetCurrentHandler()->UnloadAllFastFiles();
		ps::log::Print("MAIN", "Successfully unloaded all files.");
		ps::log::Print("MAIN", "Deinitializing...");
		ps::Parasyte::GetCurrentHandler()->Deinitialize();
		ps::log::Print("MAIN", "Handler successfully deinitialized.");
		ps::Parasyte::SetCurrentHandler(nullptr);
	}
	else if(!implicit)
	{
		ps::log::Print("WARNING", "No handler currently set to deinitialize.");
	}
}

PS_CINIT(ps::Command, ps::DeinitCommand, ps::Command::GetCommands());