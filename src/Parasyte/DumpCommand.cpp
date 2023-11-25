#include "pch.h"
#include "DumpCommand.h"
#include "ProcessDumper.h"

ps::DumpCommand::DumpCommand() : Command(
	"dump",
	"Creates a Cordycep-Compatible dump of the provided exe.",
	"This command handles automatically creating a Cordycep-Compatible dump of the exe you provide it.",
	"dump \"C:\\iw6mp64_ship.exe\"",
	true)
{

}

void ps::DumpCommand::Execute(ps::CommandParser& parser) const
{
	auto& exePath = parser.Next();

	ps::log::Print("MAIN", "Dumping: %s...", exePath.c_str());

	// Create a dumper and pass it along.
	ps::ProcessDumper dumper;

	if (!dumper.Dump(exePath))
	{
		ps::log::Print("ERROR", "Failed to dump the game, the log file has more information on why this happened.");
		ps::log::Print("ERROR", "See our documentation for more information on how to solve this.");
		return;
	}
	else
	{
		ps::log::Print("ERROR", "Successfully dumped the required data, this can now be used with the title its intended for.");
	}
}

PS_CINIT(ps::Command, ps::DumpCommand, ps::Command::GetCommands());