#pragma once
#include "Command.h"

namespace ps
{
	class RemoveFlagCommand : public Command
	{
	public:
		// Inits the command.
		RemoveFlagCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}