#pragma once

namespace ps
{
	// A class for bumping priority for a short time frame.
	class ProcessPriorityBump
	{
	private:
		// The current priority before we bumped it.
		DWORD CurrentPriority;
	public:
		// Creates a new priority bump.
		ProcessPriorityBump();
		// Kills the priority bump and returns the existing priority.
		~ProcessPriorityBump();
	};
}

