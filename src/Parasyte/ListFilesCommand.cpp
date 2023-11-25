#include "pch.h"
#include "ListFilesCommand.h"

ps::ListFilesCommand::ListFilesCommand() : Command(
	"listfiles",
	"Lists all potential files to the log.",
	"This commands lists all files within the game to the log file.",
	"listfiles",
	true)
{
}

void ps::ListFilesCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	ps::log::Print("MAIN", "Listing all files...");
	ps::Parasyte::GetCurrentHandler()->LogFiles();
	ps::log::Print("MAIN", "Successfully listed all files to the log.");
}

PS_CINIT(ps::Command, ps::ListFilesCommand, ps::Command::GetCommands());