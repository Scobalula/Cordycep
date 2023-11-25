#pragma once
#include "Command.h"

namespace ps
{
	class ClearFlagsCommand : public Command
	{
	public:
		// Inits the command.
		ClearFlagsCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}