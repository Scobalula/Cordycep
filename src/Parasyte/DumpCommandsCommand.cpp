#include "pch.h"
#include "DumpCommandsCommand.h"

ps::DumpCommandsCommand::DumpCommandsCommand() : Command(
	"dumpcommands",
	"Dumps commands to a markdown file for use on the wiki.",
	"This commmand handles dumping all commands to markdown file for use on the wiki.",
	"dumpcommands",
	false)
{
}

void ps::DumpCommandsCommand::Execute(ps::CommandParser& parser) const
{
	ps::log::Print("MAIN", "Dumping commands...");

	// Calculate the max size of the columnds
	size_t CommandSize = 0;
	size_t DescriptionSize = 0;
	size_t ExampleSize = 0;

	for (auto& command : ps::Command::GetCommands())
	{
		if (command->IsVisible())
		{
			CommandSize = max(CommandSize, command->GetName().size());
			DescriptionSize = max(DescriptionSize, command->GetDescription().size());
			ExampleSize = max(ExampleSize, command->GetExample().size());
		}
	}

	CommandSize += 4;
	DescriptionSize += 4;
	ExampleSize += 4;

	std::ofstream output("Commands.md");

	if (!output)
	{
		ps::log::Print("ERROR", "Failed to create markdown file.");
		return;
	}

	output <<
		"| " << "Command" << std::setw(CommandSize - sizeof("Command") + 1) <<
		"| " << "Description" << std::setw(DescriptionSize - sizeof("Description")) <<
		"| " << "Example" << std::setw(ExampleSize - sizeof("Example") + 3) <<
		"|\n";

	output <<
		"|" << std::setfill('-') << std::setw(CommandSize) <<
		"|" << std::setfill('-') << std::setw(DescriptionSize - 1) <<
		"|" << std::setfill('-') << std::setw(ExampleSize + 3) <<
		"|\n" << std::setfill(' ');


	for (auto& command : ps::Command::GetCommands())
	{
		if (command->IsVisible())
		{
			output <<
				"| " << command->GetName() << std::setw(CommandSize - command->GetName().size()) <<
				"| " << command->GetDescription() << std::setw(DescriptionSize - command->GetDescription().size()) <<
				"| `" << command->GetExample() << "`" << std::setw(ExampleSize - command->GetExample().size()) <<
				"|\n";
		}
	}

	ps::log::Print("MAIN", "Successfully dumped commands to a markdown file.");
}

PS_CINIT(ps::Command, ps::DumpCommandsCommand, ps::Command::GetCommands());