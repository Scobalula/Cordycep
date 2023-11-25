#include "pch.h"
#include "Helper.h"

const std::filesystem::path ps::helper::GetExePath()
{
	char path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);

	// Check if our file path is too small
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		return "";

	return path;
}

void ps::helper::SetWorkingDirectoryToExe()
{
	auto path = GetExePath().parent_path();
	std::filesystem::current_path(path);
}
