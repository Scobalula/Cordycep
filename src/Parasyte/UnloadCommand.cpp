#include "pch.h"
#include "UnloadCommand.h"

ps::UnloadCommand::UnloadCommand() : Command(
	"unload",
	"Unloads a file with the given name.",
	"This command unloads the file with the provided name.",
	"unload mp_frontend_jup_01",
	true)
{
}

void ps::UnloadCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();
	auto& name = parser.Next();
	ps::log::Print("MAIN", "Unloading %s...", name.c_str());

	if (ps::Parasyte::GetCurrentHandler()->UnloadFastFile(name))
	{
		ps::log::Print("MAIN", "Successfully unloaded: %s and all its assets.", name.c_str());
	}
	else
	{
		ps::log::Print("ERROR", "File: %s does not exist, make sure the file is loaded and the name you provided is correct.", name.c_str());
	}
}

PS_CINIT(ps::Command, ps::UnloadCommand, ps::Command::GetCommands());