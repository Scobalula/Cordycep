#pragma once
#include "Command.h"

namespace ps
{
	class ActivateCommand : public Command
	{
	public:
		// Inits the command.
		ActivateCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}