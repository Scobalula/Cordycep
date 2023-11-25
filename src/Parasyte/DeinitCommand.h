#pragma once
#include "Command.h"

namespace ps
{
	class DeinitCommand : public Command
	{
	public:
		// Inits the command.
		DeinitCommand();

		// Executes the command
		void Execute(ps::CommandParser& parser) const;

		// Deinitializes the current handler.
		static void DeinitializeCurrentHandler(bool implicit);
	};
}