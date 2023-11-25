#pragma once
#include "FileSystem.h"

namespace ps
{
	// A class to hold a safe file handle that automatically closes itself.
	class FileHandle
	{
	private:
		// The file system this handle was opened from.
		ps::FileSystem* FileSystem;
		// The actual file handle.
		HANDLE Handle;
	public:
		// Creates a new file handle.
		FileHandle(HANDLE handle, ps::FileSystem* fileSystem);
		// Destroys the file handle.
		~FileHandle();
		// Checks if the file handle is valid.
		const bool IsValid() const;
		// Reads data from the file.
		std::unique_ptr<uint8_t[]> Read(const size_t size);
		// Reads data from the file.
		size_t Read(uint8_t* buffer, const size_t size);
		// Reads data from the file.
		size_t Read(uint8_t* buffer, const size_t offset, const size_t size);
		// Reads data from the file.
		size_t Write(const uint8_t* buffer, const size_t size);
		// Reads data from the file.
		size_t Write(const uint8_t* buffer, const size_t offset, const size_t size);
		// Tells the file position.
		size_t Tell();
		// Seeks within a file.
		size_t Seek(size_t position, size_t direction);
		// Gets the size of the file.
		size_t Size();
		// Reads data from the file.
		template <class T>
		T Read()
		{
			T r{};
			Read((uint8_t*)&r, 0, sizeof(r));
			return r;
		}
		// Reads data from the file.
		template <class T>
		std::unique_ptr<T[]> ReadArray(const size_t count)
		{
			auto r = std::make_unique<T[]>(count);
			Read((uint8_t*)&r[0], 0, count * sizeof(T));
			return r;
		}
		// Closes the file handle.
		void Close();
		// Gets the file handle.
		HANDLE GetHandle();
	};
}
