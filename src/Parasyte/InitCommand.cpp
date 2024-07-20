#include "pch.h"
#include "InitCommand.h"

ps::InitCommand::InitCommand() : Command(
	"init",
	"Initializes the current handler with a given game path.",
	"This command handles initializing the current handler, which may include loading the game's module and any other data required.",
	"init \"C:\\Path\\To\\Game\"",
	true)
{
}

void ps::InitCommand::Execute(ps::CommandParser& parser) const
{
	if (ps::Parasyte::GetCurrentHandler() == nullptr)
	{
		throw std::exception("A handler has not been set, did you forget to call \"sethandler\"? Type: help to get some usage info.");
	}

	auto& gameDirectory = parser.Next();
	ps::log::Print("MAIN", "Attempting to initialize: %s, please wait...", ps::Parasyte::GetCurrentHandler()->GetName().c_str());
	std::ofstream output("Data\\CurrentHandler.csi", std::ios::binary);

	if (!output)
	{
		ps::log::Print("ERROR", "Failed to create the state file, this can happen if your file path is too long or Cordycep is denied.");
		ps::log::Print("ERROR", "Try moving Cordycep's exe closer to the root of your drive and ensuring nothing is reading/writing from it.");
		throw std::exception("Initialization failed.");
	}

	if (ps::Parasyte::GetCurrentHandler()->Initialize(gameDirectory))
	{
		ps::log::Print("MAIN", "The handler: %s initialized successfully.", ps::Parasyte::GetCurrentHandler()->GetName().c_str());

		auto poolsAddress   = (uintptr_t)ps::Parasyte::GetCurrentHandler()->XAssetPools.get();
		auto stringsAddress = (uintptr_t)ps::Parasyte::GetCurrentHandler()->Strings.get();
		auto stringSize     = (uint32_t)gameDirectory.size();
		auto& gameDirectory = ps::Parasyte::GetCurrentHandler()->GameDirectory;
		auto numFlags       = (uint32_t)ps::Parasyte::GetCurrentHandler()->Flags.size();

		output.write((const char*)&ps::Parasyte::GetCurrentHandler()->ID, sizeof(ps::Parasyte::GetCurrentHandler()->ID));
		output.write((const char*)&poolsAddress, sizeof(poolsAddress));
		output.write((const char*)&stringsAddress, sizeof(stringsAddress));
		output.write((const char*)&stringSize, sizeof(stringSize));
		output.write((const char*)gameDirectory.c_str(), gameDirectory.size());
		output.write((const char*)&numFlags, sizeof(numFlags));

		for (auto& flag : ps::Parasyte::GetCurrentHandler()->Flags)
		{
			auto flagSize = (uint32_t)flag.size();

			output.write((const char*)&flagSize, sizeof(flagSize));
			output.write((const char*)flag.c_str(), flag.size());
		}
	}
	else
	{
		ps::Parasyte::GetCurrentHandler()->Deinitialize();
		ps::log::Print("ERROR", "The handler failed to initialize, the log file has more information on why this happened.");
		ps::log::Print("ERROR", "Check the game path provided and ensure you've dumped the game correctly.");
		throw std::exception("Initialization failed.");
	}
}

PS_CINIT(ps::Command, ps::InitCommand, ps::Command::GetCommands());