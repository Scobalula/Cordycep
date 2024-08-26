#pragma once
#include "Command.h"

namespace ps
{
	class DecryptScriptCommand : public Command
	{
	public:
		// Inits the command.
		DecryptScriptCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}