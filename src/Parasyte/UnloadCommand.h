#pragma once
#include "Command.h"

namespace ps
{
	class UnloadCommand : public Command
	{
	public:
		// Inits the command.
		UnloadCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}