#include "pch.h"
#include "DumpAliasesCommand.h"

ps::DumpAliasesCommand::DumpAliasesCommand() : Command(
	"dumpaliases",
	"Dumps all supported items as aliases into a config json.",
	"Dumps all supported items as aliases into a config json.",
	"dumpaliases",
	true)
{
}

void ps::DumpAliasesCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	//auto& fileName = parser.Next();
	ps::Parasyte::DumpAliases();
}

PS_CINIT(ps::Command, ps::DumpAliasesCommand, ps::Command::GetCommands());