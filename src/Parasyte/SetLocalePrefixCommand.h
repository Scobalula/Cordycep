#pragma once
#include "Command.h"

namespace ps
{
	class SetLocalePrefixCommand : public Command
	{
	public:
		// Inits the command.
		SetLocalePrefixCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}