#pragma once

namespace ps
{
	class XAssetPackageRef
	{
	private:
		// Current file handle.
		HANDLE FileHandle;
	public:
		// Creates a new package reference.
		XAssetPackageRef();
		// Creates a new package reference.
		XAssetPackageRef(const std::string& fileName);
		// Creates a new package reference.
		XAssetPackageRef(const HANDLE storageHandle, const std::string& fileName);
		// Creates a new package reference.
		XAssetPackageRef(HANDLE handle);

		// Checks if the package reference is valid.
		bool IsValid() { return FileHandle != NULL && FileHandle != INVALID_HANDLE_VALUE; }
		// Gets the file handle.
		HANDLE GetHandle() { return FileHandle; }

		// Closes the package reference.
		~XAssetPackageRef();
	};
}
