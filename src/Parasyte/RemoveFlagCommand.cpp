#include "pch.h"
#include "RemoveFlagCommand.h"

ps::RemoveFlagCommand::RemoveFlagCommand() : Command(
	"removeflag",
	"Removes the provided flag within the handler.",
	"This command handles removing the game specific flags within the handler.",
	"removeflag sp",
	true)
{
}

void ps::RemoveFlagCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler(false);

	auto& flag = parser.Next();
	ps::Parasyte::GetCurrentHandler()->RemoveFlag(flag);
	ps::log::Print("MAIN", "Successfully removed game flag: %s", flag.c_str());
}

PS_CINIT(ps::Command, ps::RemoveFlagCommand, ps::Command::GetCommands());