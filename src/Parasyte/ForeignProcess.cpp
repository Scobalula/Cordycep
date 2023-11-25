#include "pch.h"
#include "ForeignProcess.h"

ps::ForeignProcess::ForeignProcess()
{
	Handle = NULL;
	ID = 0;
}

ps::ForeignProcess::ForeignProcess(size_t id)
{
	Handle = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)id);
	ID = id;
}

ps::ForeignProcess::ForeignProcess(HANDLE handle, size_t id)
{
	Handle = handle;
	ID = id;
}

ps::ForeignProcess::ForeignProcess(HANDLE handle)
{
	Handle = handle;
	ID = GetProcessId(handle);
}

ps::ForeignProcess::~ForeignProcess()
{
	CloseHandle(Handle);
}

const uintptr_t ps::ForeignProcess::GetMainModuleAddress() const
{
	if (Handle == NULL)
		return 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, (DWORD)ID);

	if (snapshot == NULL)
		return 0;

	uintptr_t result = 0;
	MODULEENTRY32 moduleEntry { sizeof(MODULEENTRY32) };

	if (Module32First(snapshot, &moduleEntry))
		result = (uintptr_t)moduleEntry.modBaseAddr;

	CloseHandle(snapshot);
	return result;
}

const size_t ps::ForeignProcess::GetMainModuleSize() const
{
	if (Handle == NULL)
		return 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, (DWORD)ID);

	if (snapshot == NULL)
		return 0;

	size_t result = 0;
	MODULEENTRY32 moduleEntry{ sizeof(MODULEENTRY32) };

	if (Module32First(snapshot, &moduleEntry))
		result = (size_t)moduleEntry.modBaseSize;

	CloseHandle(snapshot);
	return result;
}

const uintptr_t ps::ForeignProcess::GetCodeSection(const uintptr_t moduleAddress) const
{
	auto oldHeader = Read<IMAGE_DOS_HEADER>(moduleAddress);
	auto newHeader = Read<IMAGE_NT_HEADERS>(moduleAddress + oldHeader.e_lfanew);
	auto textSection = Read<IMAGE_SECTION_HEADER>(moduleAddress + oldHeader.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + newHeader.FileHeader.SizeOfOptionalHeader);

	return (uintptr_t)(moduleAddress + textSection.VirtualAddress);
}

const size_t ps::ForeignProcess::GetCodeSectionSize(const uintptr_t moduleAddress) const
{
	auto oldHeader = Read<IMAGE_DOS_HEADER>(moduleAddress);
	auto newHeader = Read<IMAGE_NT_HEADERS>(moduleAddress + oldHeader.e_lfanew);
	auto textSection = Read<IMAGE_SECTION_HEADER>(moduleAddress + oldHeader.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + newHeader.FileHeader.SizeOfOptionalHeader);

	return textSection.Misc.VirtualSize;
}

const uintptr_t ps::ForeignProcess::GetEndOfCodeSection(const uintptr_t moduleAddress) const
{
	auto oldHeader = Read<IMAGE_DOS_HEADER>(moduleAddress);
	auto newHeader = Read<IMAGE_NT_HEADERS>(moduleAddress + oldHeader.e_lfanew);
	auto textSection = Read<IMAGE_SECTION_HEADER>(moduleAddress + oldHeader.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + newHeader.FileHeader.SizeOfOptionalHeader);

	return (uintptr_t)(moduleAddress + textSection.VirtualAddress + textSection.Misc.VirtualSize);
}

const uintptr_t ps::ForeignProcess::GetEntryPoint(const uintptr_t moduleAddress) const
{
	auto oldHeader = Read<IMAGE_DOS_HEADER>(moduleAddress);
	auto newHeader = Read<IMAGE_NT_HEADERS>(moduleAddress + oldHeader.e_lfanew);

	return newHeader.OptionalHeader.AddressOfEntryPoint;
}

const uintptr_t ps::ForeignProcess::GetForeignProcessAddress(const uintptr_t moduleAddress, const std::string& name) const
{
	auto oldHeader    = Read<IMAGE_DOS_HEADER>(moduleAddress);
	auto newHeader    = Read<IMAGE_NT_HEADERS>(moduleAddress + oldHeader.e_lfanew);
	auto exportsDir   = newHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	auto exports      = Read<IMAGE_EXPORT_DIRECTORY>(moduleAddress + exportsDir.VirtualAddress);

	for (uint32_t i = 0; i < exports.NumberOfNames; i++)
	{
		auto exportName = ReadString(moduleAddress + (uintptr_t)Read<DWORD>(moduleAddress + exports.AddressOfNames + i * sizeof(DWORD)), 1024);

		if (exportName == name)
		{
			auto ordinal = Read<WORD>(moduleAddress + exports.AddressOfNameOrdinals + i * sizeof(WORD));
			auto address = Read<DWORD>(moduleAddress + exports.AddressOfFunctions + ordinal * sizeof(DWORD));

			//Function is forwarded
			if (address > newHeader.OptionalHeader.DataDirectory[0].VirtualAddress && address < (newHeader.OptionalHeader.DataDirectory[0].VirtualAddress + newHeader.OptionalHeader.DataDirectory[0].Size))
			{
				return 0;
			}
			else
			{
				return moduleAddress + (uintptr_t)address;
			}
		}
	}

	return 0;
}

std::unique_ptr<uint8_t[]> ps::ForeignProcess::ReadCodeSection(const uintptr_t address, const size_t size) const
{
	auto codeSectionPtr = GetCodeSection(address);
	auto codeSectionBuf = std::make_unique<uint8_t[]>(size);
	size_t codeSectionSize = size;
	size_t codeSectionConsumed = 0;

	if (Read(codeSectionPtr, codeSectionBuf.get(), codeSectionSize) != codeSectionSize)
	{
		return nullptr;
	}

	return codeSectionBuf;
}

size_t  ps::ForeignProcess::Read(const uintptr_t address, uint8_t* buffer, const size_t size) const
{
	SIZE_T read = 0;
	if (!ReadProcessMemory(Handle, (LPCVOID)address, buffer, size, &read))
	{
		return 0;
	}
	return read;
}

std::unique_ptr<uint8_t[]> ps::ForeignProcess::Read(const uintptr_t address, const size_t size) const
{
	auto result = std::make_unique<uint8_t[]>(size);
	if (Read(address, result.get(), size) != size)
	{
		return nullptr;
	}
	return result;
}

std::string ps::ForeignProcess::ReadString(const uintptr_t address, const size_t maxSize) const
{
	std::string result;
	size_t i = 0;

	for (size_t i = 0; i < maxSize; i++)
	{
		char c = 0;

		if (!ReadProcessMemory(Handle, (LPCVOID)(address + i), &c, sizeof(c), NULL))
		{
			break;
		}
		if (c == 0)
		{
			break;
		}

		result.push_back(c);
	}

	return result;
}

size_t ps::ForeignProcess::Write(const uintptr_t address, uint8_t* buffer, const size_t size) const
{
	SIZE_T read = 0;
	if (!WriteProcessMemory(Handle, (LPVOID)address, buffer, size, &read))
	{
		return 0;
	}
	return read;
}

class ForeignThread
{
private:
	// A handle to the thread.
	HANDLE Handle;
	// The id of the thread within the operating system.
	size_t ID;
public:
	// Creates a new foreign thread.
	ForeignThread();
	// Creates a new foreign thread.
	ForeignThread(size_t id);
	// Creates a new foreign thread.
	ForeignThread(HANDLE handle, size_t id);
	// Destorys the foreign thread.
	~ForeignThread();

	// Closes the thread.
	const bool Close();
	// Checks if the process is still open and running.
	const bool IsOpen() const;
	// Checks if the process is valid.
	const bool IsValid() const;
	// Gets the process Handle.
	const HANDLE GetHandle() const;
	// Gets the process ID.
	const size_t GetID() const;
};

const size_t ps::ForeignProcess::ThreadCount() const
{
	size_t c = 0;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (snap != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 thread{ .dwSize = sizeof(THREADENTRY32) };

		if (Thread32First(snap, &thread))
		{
			do
			{
				if (thread.th32OwnerProcessID == GetID())
				{
					c++;
				}
			} while (Thread32Next(snap, &thread));
		}

		CloseHandle(snap);
	}

	return c;
}

const bool ps::ForeignProcess::EndAllThreads() const
{
	bool success = false;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (snap != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 thread{ .dwSize = sizeof(THREADENTRY32) };

		if (Thread32First(snap, &thread))
		{
			do
			{
				if (thread.th32OwnerProcessID == GetID())
				{
					ForeignThread t(thread.th32ThreadID);

					t.Close();
				}
			} while (Thread32Next(snap, &thread));
		}

		CloseHandle(snap);
	}

	return success;
}

const bool ps::ForeignProcess::IsOpen() const
{
	return Handle != NULL && WaitForSingleObject(Handle, 0) == WAIT_TIMEOUT;
}

const bool ps::ForeignProcess::IsValid() const
{
	return Handle == NULL || Handle == INVALID_HANDLE_VALUE;
}

const HANDLE ps::ForeignProcess::GetHandle() const
{
	return Handle;
}

const size_t ps::ForeignProcess::GetID() const
{
	return ID;
}

void ps::ForeignProcess::EnumerateProcessesWithName(const std::string& path, std::function<void(DWORD)> callback)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (snap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe32{ .dwSize = sizeof(PROCESSENTRY32) };

		if (Process32First(snap, &pe32))
		{
			do
			{
				if (!_stricmp(path.c_str(), pe32.szExeFile))
				{
					try { callback(pe32.th32ProcessID); }
					catch (...) {}
				}
			} while (Process32Next(snap, &pe32));
		}

		CloseHandle(snap);
	}
}

ForeignThread::ForeignThread()
{
	Handle = NULL;
	ID = 0;
}

ForeignThread::ForeignThread(size_t id)
{
	Handle = OpenThread(PROCESS_ALL_ACCESS, false, (DWORD)id);
	ID = id;
}

ForeignThread::ForeignThread(HANDLE handle, size_t id)
{
	Handle = handle;
	ID = id;
}

ForeignThread::~ForeignThread()
{
	CloseHandle(Handle);
}

const bool ForeignThread::Close()
{
	TerminateThread(Handle, 0);
	CloseHandle(Handle);
	Handle = NULL;
	return true;
}

const bool ForeignThread::IsOpen() const
{
	return false;
}

const bool ForeignThread::IsValid() const
{
	return false;
}

const HANDLE ForeignThread::GetHandle() const
{
	return Handle;
}

const size_t ForeignThread::GetID() const
{
	return ID;
}
