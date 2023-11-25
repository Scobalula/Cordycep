#pragma once
#include "Command.h"

namespace ps
{
	class EnableVerbosityCommand : public Command
	{
	public:
		// Inits the command.
		EnableVerbosityCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}