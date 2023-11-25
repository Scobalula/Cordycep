#include "pch.h"
#include "LoadListCommand.h"

ps::LoadListCommand::LoadListCommand() :Command(
	"loadlist",
	"Loads files using a list file.",
	"This command loads files from a txt file with each file name on each line of the file.",
	"loadlist C:\\list.txt",
	true)
{
}

void ps::LoadListCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();

	auto& fileName = parser.Next();
	std::vector<std::string> files;

	ps::log::Print("MAIN", "Parsing file list, please wait...");

	std::ifstream file(fileName);
	std::string line;

	while (std::getline(file, line))
	{
		files.push_back(line);
	}

	if (files.size() == 0)
	{
		ps::log::Print("ERROR", "Failed to find files in file list.");
		return;
	}

	ps::log::Print("MAIN", "Parsed %llu files.", files.size());
	ps::log::Print("MAIN", "Loading %llu files, please wait...", files.size());

	size_t idx = 1;
	size_t count = files.size();

	for (auto& fileName : files)
	{
		ps::Parasyte::LoadFile(fileName, idx, count, FastFileFlags::None);
		idx++;
	}

	ps::log::Print("MAIN", "Loaded %llu files matching pattern.", files.size());
}

PS_CINIT(ps::Command, ps::LoadListCommand, ps::Command::GetCommands());