#include "pch.h"
#include "Parasyte.h"
#include "ProcessDumper.h"
#include "ProcessPriorityBump.h"
#include "Utility.h"
#include "shellapi.h"
#include "Helper.h"

// For manipulating PE Module
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

ps::ProcessDumper::ProcessDumper()
{
	NtSuspendProcess = NULL;
	NtResumeProcess = NULL;
}

bool ps::ProcessDumper::Initialize()
{
	// Load NTDLL, note: unlike Scylla, we're not taking control
	// of NTDLL, and it can remain loaded for entire duration.
	auto ntdll = LoadLibraryA("ntdll.dll");

	if (ntdll == NULL)
	{
		return false;
	}

	NtSuspendProcess = (decltype(NtSuspendProcess))GetProcAddress(ntdll, "NtSuspendProcess");
	NtResumeProcess = (decltype(NtResumeProcess))GetProcAddress(ntdll, "NtResumeProcess");

	return NtSuspendProcess != NULL && NtResumeProcess != NULL;
}

bool ps::ProcessDumper::Suspend(const HANDLE processHandle)
{
	return NtSuspendProcess != NULL && NT_SUCCESS(NtSuspendProcess(processHandle));
}

bool ps::ProcessDumper::Resume(const HANDLE processHandle)
{
	return NtResumeProcess != NULL && NT_SUCCESS(NtResumeProcess(processHandle));
}

BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	LUID luid;
	BOOL bRet = FALSE;

	if (LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		TOKEN_PRIVILEGES tp{};

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;
		//
		//  Enable the privilege or disable all privileges.
		//
		if (AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
		{
			//
			//  Check to see if you have proper access.
			//  You may get "ERROR_NOT_ALL_ASSIGNED".
			//
			bRet = (GetLastError() == ERROR_SUCCESS);
		}
	}
	return bRet;
}

bool ps::ProcessDumper::Dump(const std::string& path)
{
	// Bump our priority so we have more priority over stuff going on.
	ps::ProcessPriorityBump bump;

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hToken;

	if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
		CloseHandle(hToken);
	}

	ps::log::Log(ps::LogType::Normal, "Dumping: %s...", path.c_str());

	if (!Interface.Initialize())
	{
		ps::log::Log(ps::LogType::Error, "Failed to initialize Scylla, have you correctly copied the required DLLs into Data\\Deps?");
		return false;
	}
	if (!Initialize())
	{
		ps::log::Log(ps::LogType::Error, "Failed to initialize the game dumper, are you on a supported version of Windows?");
		return false;
	}

	ps::log::Log(ps::LogType::Normal, "Successfully initialized the dumper and Scylla.");

	std::filesystem::path asPath = path;
	std::filesystem::path fnPath = asPath.filename();

	SHELLEXECUTEINFOA execInfo = { 0 };
	execInfo.cbSize      = sizeof(SHELLEXECUTEINFOA);
	execInfo.fMask       = SEE_MASK_NOCLOSEPROCESS;
	execInfo.hwnd        = NULL;
	execInfo.lpVerb      = NULL;
	execInfo.lpFile      = path.c_str();
	execInfo.lpDirectory = NULL;
	execInfo.nShow       = SW_SHOW;
	execInfo.hInstApp    = NULL;

	auto valid = true;

	// Create process variables
	STARTUPINFO si{ .cb = sizeof(si) };
	PROCESS_INFORMATION pi{};

	// Create the process in a suspended state
	if (CreateProcess(NULL, const_cast<LPSTR>(path.c_str()), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi))
	{
		ps::ForeignProcess foreignProcess(pi.hProcess, pi.dwProcessId);

		DEBUG_EVENT dbgEvent = {};

		for (;;)
		{
			if (!WaitForDebugEvent(&dbgEvent, 30000))
			{
				foreignProcess.EndAllThreads();
				continue;
			}

			// If we're quiting threads, we're more than likely unpacked
			// and the game is attempting to exit (either not detecting platform
			// or detecting us)
			if (dbgEvent.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
			{
				auto eoc = foreignProcess.GetEndOfCodeSection(foreignProcess.GetMainModuleAddress());
				auto cur = foreignProcess.Read<uint64_t>(eoc - 8);

				if (cur != 0)
				{
					auto code = Interface.Dump(foreignProcess, ((std::filesystem::path)"Data" / "Dumps" / (fnPath.replace_extension("").string() + "_dump.exe")).string());

					if (code != 0)
					{
						ps::log::Log(ps::LogType::Error, "Scylla encountered an error while dumping the process.");
						valid = false;
					}
				}
				else
				{
					ps::log::Log(ps::LogType::Error, "The game cleared its memory before we could finish dumping.");
					ps::log::Log(ps::LogType::Error, "Try dumping the game again.");
					valid = false;
				}

				// Done debugging.
				DebugActiveProcessStop(dbgEvent.dwProcessId);
				break;
			}

			if (dbgEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
			{
				// If we're an exception or illegal instruction, quit
				if (dbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION || dbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
				{
					auto eoc = foreignProcess.GetEndOfCodeSection(foreignProcess.GetMainModuleAddress());
					auto cur = foreignProcess.Read<uint64_t>(eoc - 8);

					if (cur != 0)
					{
						auto code = Interface.Dump(foreignProcess, ((std::filesystem::path)"Data" / "Dumps" / (fnPath.replace_extension("").string() + "_dump.exe")).string());

						if (code != 0)
						{
							ps::log::Log(ps::LogType::Error, "Scylla encountered an error while dumping the process.");
							valid = false;
						}
					}
					else
					{
						ps::log::Log(ps::LogType::Error, "The game cleared its memory before we could finish dumping.");
						ps::log::Log(ps::LogType::Error, "Try dumping the game again.");
						valid = false;
					}

					// Done debugging.
					DebugActiveProcessStop(dbgEvent.dwProcessId);
					break;
				}
			}

			ContinueDebugEvent(dbgEvent.dwProcessId, dbgEvent.dwThreadId, DBG_CONTINUE);
		}

		// Close the handles
		CloseHandle(pi.hThread);
	}
	else
	{
		ps::log::Log(ps::LogType::Error, "Scylla encountered an error while dumping the process.");
		valid = false;
	}

	// Kill left over processes spawned.
	ps::ForeignProcess::EnumerateProcessesWithName(fnPath.string(), [](DWORD id)
	{
		ps::ForeignProcess fp(id);
		fp.EndAllThreads();
	});

	return valid;
}
