#include "pch.h"
#include "HelpCommand.h"

ps::HelpCommand::HelpCommand() : Command(
	"help",
	"Shows the help message.",
	"This commands shows the help message which shows commands, handlers and tips.",
	"help",
	true)
{
}

void ps::HelpCommand::Execute(ps::CommandParser& parser) const
{
	ps::log::Print("HELP", "To use Cordycep, you can simply pass it commands to tell it what to do.");
	ps::log::Print("HELP", "Here's a list of commands:");

	for (const auto& command : ps::Command::GetCommands())
	{
		if (command->IsVisible())
		{
			ps::log::Print("HELP", "\t%-32s %s", command->GetName().c_str(), command->GetDescription().c_str());
		}
	}

	ps::log::Print("HELP", "Need a quick start? To initialize a game: sethandler <Handler Name> init <Game Path> setlocaleprefix <Locale Prefix> loadcommonfiles");
	ps::log::Print("HELP", "For example: sethandler mw4 init \"C:\\Battlenet\\Call of Duty Modern Warfare\" setlocaleprefix \"english\\eng_\" loadcommonfiles");
	ps::log::Print("HELP", "To load individual files: load <File Name>, for example: load mp_shipment");
	ps::log::Print("HELP", "To load a list of files: loadlist C:\\List.txt");
	ps::log::Print("HELP", "To load files using pattern/wildcards: loadwc <pattern>, for example: loadwc *mpapa5*");
	ps::log::Print("HELP", "To load files using aliases: loadalias <item name>, for example: loadalias \"MP-5\"");
	ps::log::Print("HELP", "Here's a list of handlers Cordycep currently supports:");

	for (auto& handler : ps::GameHandler::GetHandlers())
	{
		ps::log::Print("HELP", "\t%-12s %s", handler->GetShorthand().c_str(), handler->GetName().c_str());
	}

	ps::log::Print("HELP", "When reporting bugs with Cordycep, it is vital you provide a full fog file.");
	ps::log::Print("HELP", "Failure to provide the full log file will result in your bug report being ignored.");
}

PS_CINIT(ps::Command, ps::HelpCommand, ps::Command::GetCommands());