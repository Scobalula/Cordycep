#pragma once
#include "Command.h"

namespace ps
{
	class SetFlagCommand : public Command
	{
	public:
		// Inits the command.
		SetFlagCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}