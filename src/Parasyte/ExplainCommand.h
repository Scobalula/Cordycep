#pragma once
#include "Command.h"

namespace ps
{
	class ExplainCommand : public Command
	{
	public:
		// Inits the command.
		ExplainCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}