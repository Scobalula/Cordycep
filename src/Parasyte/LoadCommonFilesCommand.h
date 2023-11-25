#pragma once
#include "Command.h"

namespace ps
{
	class LoadCommonFilesCommand : public Command
	{
	public:
		// Inits the command.
		LoadCommonFilesCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;
	};
}