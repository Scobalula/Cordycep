#pragma once
#include "Command.h"

namespace ps
{
	class ListLoadedFilesCommand : public Command
	{
	public:
		// Inits the command.
		ListLoadedFilesCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}