#pragma once
#include "Command.h"

namespace ps
{
	class UnloadAliasCommand : public Command
	{
	public:
		// Inits the command.
		UnloadAliasCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}