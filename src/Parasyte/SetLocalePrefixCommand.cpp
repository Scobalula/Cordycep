#include "pch.h"
#include "SetLocalePrefixCommand.h"

ps::SetLocalePrefixCommand::SetLocalePrefixCommand() : Command(
	"setlocaleprefix",
	"Sets the game specific locale prefix.",
	"This command handles setting locale prefix to ensure localized files specific to you and the game get loaded.",
	"setlocaleprefix english\\en_",
	true)
{
}

void ps::SetLocalePrefixCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	auto& locale = parser.Next();
	ps::Parasyte::GetCurrentHandler()->SetRegionPrefix(locale);
	ps::log::Print("MAIN", "Successfully set the locale prefix to: %s", locale.c_str());
}

PS_CINIT(ps::Command, ps::SetLocalePrefixCommand, ps::Command::GetCommands());