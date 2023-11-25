#include "pch.h"
#include "Parasyte.h"
#include "SetHandlerCommand.h"
#include "DeinitCommand.h"
#include "GameHandler.h"

ps::SetHandlerCommand::SetHandlerCommand() : Command(
	"sethandler",
	"Sets the current handler.",
	"This command handles setting the current handler. This command also shuts down the current handler and clears it from memory.",
	"sethandler mw4",
	true)
{

}

void ps::SetHandlerCommand::Execute(ps::CommandParser& parser) const
{
	ps::DeinitCommand::DeinitializeCurrentHandler(true);

	auto& handlerName = parser.Next();

	ps::log::Print("MAIN", "Looking for a handler for: %s...", handlerName.c_str());

	auto handler = ps::GameHandler::FindHandler(handlerName);

	if (handler == nullptr)
	{
		throw std::exception("Failed to find a game handler with the given name.");
	}
	else
	{
		ps::Parasyte::SetCurrentHandler(handler);
		ps::log::Print("MAIN", "Successfully set game handler to: %s", ps::Parasyte::GetCurrentHandler()->GetName().c_str());
	}
}

PS_CINIT(ps::Command, ps::SetHandlerCommand, ps::Command::GetCommands());