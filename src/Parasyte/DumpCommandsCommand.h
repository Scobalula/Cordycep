#pragma once
#include "Command.h"

namespace ps
{
	class DumpCommandsCommand : public Command
	{
	public:
		// Inits the command.
		DumpCommandsCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}