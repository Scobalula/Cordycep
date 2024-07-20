#pragma once
#include "Command.h"

namespace ps
{
	class DumpAliasesCommand : public Command
	{
	public:
		// Inits the command.
		DumpAliasesCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}