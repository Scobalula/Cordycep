#include "pch.h"
#include "ProcessPriorityBump.h"

ps::ProcessPriorityBump::ProcessPriorityBump()
{
	CurrentPriority = GetPriorityClass(GetCurrentProcess());
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

ps::ProcessPriorityBump::~ProcessPriorityBump()
{
	SetPriorityClass(GetCurrentProcess(), CurrentPriority);
}
