#pragma once
#include "Command.h"

namespace ps
{
	class LoadListCommand : public Command
	{
	public:
		// Inits the command.
		LoadListCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}