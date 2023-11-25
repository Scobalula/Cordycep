#pragma once
#include "Command.h"

namespace ps
{
	class InitCommand : public Command
	{
	public:
		// Inits the command.
		InitCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}