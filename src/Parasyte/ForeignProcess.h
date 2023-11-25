#pragma once
namespace ps
{
	// A class to hold a foreign process.
	class ForeignProcess
	{
	private:
		// A handle to the process.
		HANDLE Handle;
		// The id of the process within the operating system.
		size_t ID;
	public:
		// Creates a new foreign process.
		ForeignProcess();
		// Creates a new foreign process.
		ForeignProcess(size_t id);
		// Creates a new foreign process.
		ForeignProcess(HANDLE handle, size_t id);
		// Creates a new foreign process.
		ForeignProcess(HANDLE handle);
		// Destorys the foreign process.
		~ForeignProcess();

		// Gets the address of the main module.
		const uintptr_t GetMainModuleAddress() const;
		// Gets the size of the main module.
		const size_t GetMainModuleSize() const;
		// Gets the start of the code section.
		const uintptr_t GetCodeSection(const uintptr_t moduleAddress) const;
		// Gets the size of the code section.
		const size_t GetCodeSectionSize(const uintptr_t moduleAddress) const;
		// Gets the end of the code section.
		const uintptr_t GetEndOfCodeSection(const uintptr_t moduleAddress) const;
		// Gets the entry point.
		const uintptr_t GetEntryPoint(const uintptr_t moduleAddress) const;
		// Gets the entry point.
		const uintptr_t GetForeignProcessAddress(const uintptr_t moduleAddress, const std::string& name) const;
		// Reads the code section.
		std::unique_ptr<uint8_t[]> ReadCodeSection(const uintptr_t address, const size_t size) const;
		// Reads a data buffer from the memory of the process.
		size_t Read(const uintptr_t address, uint8_t* buffer, const size_t size) const;
		// Reads a data buffer from the memory of the process.
		std::unique_ptr<uint8_t[]> Read(const uintptr_t address, const size_t size) const;
		// Reads a string terminated by a null byte.
		std::string ReadString(const uintptr_t address, const size_t maxSize) const;
		// Writes a data buffer to the memory of the process.
		size_t Write(const uintptr_t address, uint8_t* buffer, const size_t size) const;
		// Reads a value from the memory of the process.
		template <class T>
		T Read(const uintptr_t address) const
		{
			T result{};
			ReadProcessMemory(Handle, (LPCVOID)address, &result, sizeof(result), NULL);
			return result;
		}
		// Writes a value to the memory of the process.
		template <class T>
		void Write(const uintptr_t address, const T value) const
		{
			WriteProcessMemory(Handle, (LPVOID)address, &value, sizeof(value), NULL);
		}
		// Gets the number of threads.
		const size_t ThreadCount() const;
		// Enumerates over each thread.
		const bool EndAllThreads() const;
		// Checks if the process is still open and running.
		const bool IsOpen() const;
		// Checks if the process is valid.
		const bool IsValid() const;
		// Gets the process Handle.
		const HANDLE GetHandle() const;
		// Gets the process ID.
		const size_t GetID() const;

		// Gets processes with the provided name.
		static void EnumerateProcessesWithName(const std::string& path, std::function<void(DWORD)> callback);
	};
}
