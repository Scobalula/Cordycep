#pragma once
#include "Command.h"

namespace ps
{
	class DisableVerbosityCommand : public Command
	{
	public:
		// Inits the command.
		DisableVerbosityCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}