#pragma once
#include "ScyllaInterface.h"

namespace ps
{
	// A class to handle dumping a process.
	class ProcessDumper
	{
	private:
		// Suspends the provided process.
		LONG(_stdcall* NtSuspendProcess)(HANDLE ProcessHandle);
		// Resumes the provided process.
		LONG(_stdcall* NtResumeProcess)(HANDLE ProcessHandle);
		// The Scylla Interface.
		ScyllaInterface Interface;
	public:
		// Creates a new process dumper instance.
		ProcessDumper();

		// Initializes the process dumper.
		bool Initialize();
		// Suspends the process.
		bool Suspend(const HANDLE processHandle);
		// Resumes the process.
		bool Resume(const HANDLE processHandle);
		// Dumps the provided path.
		bool Dump(const std::string& path);
	};
}
