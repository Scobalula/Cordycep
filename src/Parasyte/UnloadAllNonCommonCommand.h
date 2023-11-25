#pragma once
#include "Command.h"

namespace ps
{
	class UnloadAllNonCommonCommand : public Command
	{
	public:
		// Inits the command.
		UnloadAllNonCommonCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}