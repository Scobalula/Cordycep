#pragma once
#include "Command.h"

namespace ps
{
	class UnloadAllCommand : public Command
	{
	public:
		// Inits the command.
		UnloadAllCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}