#pragma once
#include "Command.h"

namespace ps
{
	class ListFilesCommand : public Command
	{
	public:
		// Inits the command.
		ListFilesCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}