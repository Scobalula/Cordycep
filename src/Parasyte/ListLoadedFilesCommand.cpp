#include "pch.h"
#include "ListLoadedFilesCommand.h"

ps::ListLoadedFilesCommand::ListLoadedFilesCommand() : Command(
	"listloaded",
	"Lists all files that are currently loaded.",
	"This command lists all loaded files in the current handler to the log file along with info about them.",
	"listfiles",
	false)
{
}

void ps::ListLoadedFilesCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	ps::log::Print("MAIN", "Listing all loaded files...");
	ps::Parasyte::GetCurrentHandler()->ListLoaded();
	ps::log::Print("MAIN", "Successfully listed all loaded files to the log.");
}

PS_CINIT(ps::Command, ps::ListLoadedFilesCommand, ps::Command::GetCommands());