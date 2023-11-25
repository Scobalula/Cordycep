#include "pch.h"
#include "SetFlagCommand.h"

ps::SetFlagCommand::SetFlagCommand() : Command(
	"setflag",
	"Sets the provided flag within the handler.",
	"This command handles setting the game specific flag within the handler. This could be for experimental settings or SP and MP switching.",
	"setflag sp",
	true)
{
}

void ps::SetFlagCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler(false);

	auto& flag = parser.Next();
	ps::Parasyte::GetCurrentHandler()->AddFlag(flag);
	ps::log::Print("MAIN", "Successfully set game flag: %s", flag.c_str());
}

PS_CINIT(ps::Command, ps::SetFlagCommand, ps::Command::GetCommands());