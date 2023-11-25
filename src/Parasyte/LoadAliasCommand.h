#pragma once
#include "Command.h"

namespace ps
{
	class LoadAliasCommand : public Command
	{
	public:
		// Inits the command.
		LoadAliasCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}