#pragma once
#include "Command.h"

namespace ps
{
	class MoveDirCommand : public Command
	{
	public:
		// Inits the command.
		MoveDirCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}