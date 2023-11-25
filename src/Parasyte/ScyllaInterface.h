#pragma once
#include "ForeignProcess.h"

namespace ps
{
	// A class to interface with Scylla dynamically.
	class ScyllaInterface
	{
	private:
		// Scylla's Module.
		HMODULE Module;
		// Scylla's Dump Method.
		BOOL (__stdcall *ScyllaDumpProcessA)(DWORD_PTR pid, const char* fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const char* fileResult);
	public:
		// Creates a new Scylla Interface.
		ScyllaInterface();
		// Shuts down the Scylla Interface.
		~ScyllaInterface();

		// Initializes Scylla Interface.
		const bool Initialize();
		// Dumps the provided process to the given path.
		const int Dump(const ps::ForeignProcess& foreignProcess, const std::string& path) const;
	};
}
