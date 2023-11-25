#pragma once
#include "Command.h"

namespace ps
{
	class DumpCommand : public Command
	{
	public:
		// Inits the command.
		DumpCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}