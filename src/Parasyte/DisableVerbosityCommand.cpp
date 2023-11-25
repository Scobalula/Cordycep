#include "pch.h"
#include "DisableVerbosityCommand.h"

ps::DisableVerbosityCommand::DisableVerbosityCommand() : Command(
	"disableverbosity",
	"Disables verbosity.",
	"This command disables verbosity printing.",
	"disableverbosity",
	true)
{
}

void ps::DisableVerbosityCommand::Execute(ps::CommandParser& parser) const
{
	ps::log::DisableLogType(ps::LogType::Verbose);
	ps::log::Print("MAIN", "Disabled verbosity logging.");
}

PS_CINIT(ps::Command, ps::DisableVerbosityCommand, ps::Command::GetCommands());