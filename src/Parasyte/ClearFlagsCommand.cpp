#include "pch.h"
#include "ClearFlagsCommand.h"

ps::ClearFlagsCommand::ClearFlagsCommand() : Command(
	"clearflags",
	"Clears all flags within the handler.",
	"This command handles clearing all flags within the handler.",
	"clearflags",
	true)
{
}

void ps::ClearFlagsCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler(false);

	auto& flag = parser.Next();
	ps::Parasyte::GetCurrentHandler()->ClearFlags();
	ps::log::Print("MAIN", "Successfully cleared all flags.");
}

PS_CINIT(ps::Command, ps::ClearFlagsCommand, ps::Command::GetCommands());