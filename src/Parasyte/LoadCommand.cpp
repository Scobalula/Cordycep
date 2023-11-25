#include "pch.h"
#include "LoadCommand.h"

ps::LoadCommand::LoadCommand() : Command(
	"load",
	"Loads a file with the given name.",
	"Loads a file with the given name into memory and handles loading all assets stored within it.",
	"load mp_shipment",
	true)
{
}

void ps::LoadCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	auto& fileName = parser.Next();
	ps::Parasyte::LoadFile(fileName, 1, 1, FastFileFlags::None);
}

PS_CINIT(ps::Command, ps::LoadCommand, ps::Command::GetCommands());