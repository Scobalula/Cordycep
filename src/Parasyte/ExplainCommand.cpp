#include "pch.h"
#include "ExplainCommand.h"

ps::ExplainCommand::ExplainCommand() : Command(
	"explain",
	"Explains the provided command.",
	"This command handles explaining what the other commands do!",
	"explain help explain loadcommonfiles",
	true)
{
}

void ps::ExplainCommand::Execute(ps::CommandParser& parser) const
{
	auto commandFound = false;
	auto& commandName = parser.Next();

	for (auto& command : ps::Command::GetCommands())
	{
		if (command->IsValid(commandName))
		{
			ps::log::Print("EXPLAIN", command->GetExplaination().c_str());
			ps::log::Print("EXAMPLE", command->GetExample().c_str());

			if (!command->IsVisible())
			{
				ps::log::Print("WARNING", "This command is hidden, no support is provided for it.");
			}

			commandFound = true;
			break;
		}
	}

	if (!commandFound)
	{
		throw std::exception((std::string("Unknown command to explain: ") + commandName).c_str());
	}
}

PS_CINIT(ps::Command, ps::ExplainCommand, ps::Command::GetCommands());
