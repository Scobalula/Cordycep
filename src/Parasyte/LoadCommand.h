#pragma once
#include "Command.h"

namespace ps
{
	class LoadCommand : public Command
	{
	public:
		// Inits the command.
		LoadCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}