#pragma once
#include "Command.h"

namespace ps
{
	class LoadWildCardCommand : public Command
	{
	public:
		// Inits the command.
		LoadWildCardCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}