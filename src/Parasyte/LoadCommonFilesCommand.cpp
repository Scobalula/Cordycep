#include "pch.h"
#include "LoadCommonFilesCommand.h"

ps::LoadCommonFilesCommand::LoadCommonFilesCommand() : Command(
	"loadcommonfiles",
	"Loads common files for the current handler.",
	"Loads all known common files for the current handler. These files are needed for common assets required by other files.",
	"loadcommonfiles",
	true)
{
}

void ps::LoadCommonFilesCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	auto& commonFiles = ps::Parasyte::GetCurrentHandler()->GetCommonFileNames();

	ps::log::Print("MAIN", "Loading common files, please wait...");

	size_t idx = 1;
	size_t count = commonFiles.size();

	for (auto& fileName : commonFiles)
	{
		ps::Parasyte::LoadFile(fileName, idx, count, FastFileFlags::None);
		idx++;
	}
}

PS_CINIT(ps::Command, ps::LoadCommonFilesCommand, ps::Command::GetCommands());