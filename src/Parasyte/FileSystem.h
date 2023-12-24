#pragma once

namespace ps
{
	// A class for handling a File System.
	class FileSystem
	{
	protected:
		// The directory where we are loading from.
		std::string Directory;
		// The last error code.
		size_t LastErrorCode = 0;
		// Open file handles.
		std::vector<HANDLE> OpenHandles;
		// The file system name.
		std::string Name;
	public:
		// Opens a file with the given name and mode.
		virtual HANDLE OpenFile(const std::string& fileName, const std::string& mode) = 0;
		// Closes the file.
		virtual void CloseFile(HANDLE handle);
		// Closes the handle only.
		virtual void CloseHandle(HANDLE handle) = 0;
		// Checks if the provided file exists.
		virtual bool Exists(const std::string& fileName) = 0;
		// Reads data from the file.
		virtual std::unique_ptr<uint8_t[]> Read(HANDLE handle, const size_t size);
		// Reads data from the file.
		virtual size_t Read(HANDLE handle, uint8_t* buffer, const size_t offset, const size_t size) = 0;
		// Reads data from the file.
		virtual size_t Write(HANDLE handle, const uint8_t* buffer, const size_t size);
		// Reads data from the file.
		virtual size_t Write(HANDLE handle, const uint8_t* buffer, const size_t offset, const size_t size) = 0;
		// Tells the file position.
		virtual size_t Tell(HANDLE handle) = 0;
		// Seeks within a file.
		virtual size_t Seek(HANDLE handle, size_t position, size_t direction) = 0;
		// Gets the size of the file.
		virtual size_t Size(HANDLE handle) = 0;
		// Gets the list of files matching the provided pattern.
		virtual size_t EnumerateFiles(const std::string& directory, const std::string& pattern, bool topDirectory, std::function<void(const std::string&, const size_t)> onFileFound) = 0;
		// Copies the file from the provided directory within the file system to the given on-disk directory.
		virtual bool CopyToDisk(const std::string& from, const std::string& to);
		// Gets the last error.
		size_t GetLastError() const;
		// Checks if the last call provided a valid result.
		bool IsValid() const;
		// Gets the name of the file system.
		const std::string& GetName() const;

		// Opens a directory at the given path. This method will automatically use the appropiate fs.
		static std::unique_ptr<FileSystem> Open(const std::string& dir);

		// Get the last dir name from a dir
		static std::string GetLastDirectoryName(const std::string& directory);
	};
}