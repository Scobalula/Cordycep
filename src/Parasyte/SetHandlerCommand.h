#pragma once
#include "Command.h"

namespace ps
{
	class SetHandlerCommand : public Command
	{
	public:
		// Inits the command.
		SetHandlerCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}