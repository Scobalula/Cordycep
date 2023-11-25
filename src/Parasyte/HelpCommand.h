#pragma once
#include "Command.h"

namespace ps
{
	class HelpCommand : public Command
	{
	public:
		// Inits the command.
		HelpCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}