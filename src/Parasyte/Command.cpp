#include "pch.h"
#include "Command.h"

std::list<std::unique_ptr<ps::Command>>& ps::Command::GetCommands()
{
	static std::list<std::unique_ptr<ps::Command>> commands;

	return commands;
}
