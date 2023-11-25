#include "pch.h"
#include "EnableVerbosityCommand.h"

ps::EnableVerbosityCommand::EnableVerbosityCommand() : Command(
	"enableverbosity",
	"Enables verbosity.",
	"This command enables verbosity printing, which will output more information to the log.",
	"enableverbosity",
	true)
{
}

void ps::EnableVerbosityCommand::Execute(ps::CommandParser& parser) const
{
	ps::log::EnableLogType(ps::LogType::Verbose);
	ps::log::Print("MAIN", "Enabled verbosity logging.");
}

PS_CINIT(ps::Command, ps::EnableVerbosityCommand, ps::Command::GetCommands());