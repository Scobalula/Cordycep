#include "pch.h"
#include "FileHandle.h"
#include "FileSystem.h"

#include "WinFileSystem.h"
#include "CascFileSystem.h"

void ps::FileSystem::CloseFile(HANDLE handle)
{
	for (auto it = OpenHandles.begin(); it != OpenHandles.end(); )
	{
		if (*it == handle)
		{
			it = OpenHandles.erase(it);
		}
		else
		{
			it++;
		}
	}

	CloseHandle(handle);
}

std::unique_ptr<uint8_t[]> ps::FileSystem::Read(HANDLE handle, const size_t size)
{
	auto result = std::make_unique<uint8_t[]>(size);

	if (Read(handle, result.get(), 0, size) != size)
	{
		return nullptr;
	}

	return result;
}

size_t ps::FileSystem::Write(HANDLE handle, const uint8_t* buffer, const size_t size)
{
	return Write(handle, buffer, 0, size);
}

bool ps::FileSystem::CopyToDisk(const std::string& from, const std::string& to)
{
	auto handle = ps::FileHandle(OpenFile(from, "r"), this);

	if (handle.GetHandle() == NULL || handle.GetHandle() == INVALID_HANDLE_VALUE)
		return false;

	auto output = std::ofstream(to, std::ios::binary);

	if (!output)
		return false;

	const size_t scratchSize = 65535;
	auto scratch = std::make_unique<uint8_t[]>(scratchSize);

	while (true)
	{
		auto sizeRead = Read(handle.GetHandle(), scratch.get(), 0, scratchSize);

		if (sizeRead == 0)
			break;
		if (!output.write((const char*)scratch.get(), sizeRead))
			return false;
		if (sizeRead != scratchSize)
			break;
	}

	return true;
}

const size_t ps::FileSystem::GetLastError() const
{
	return LastErrorCode;
}

const bool ps::FileSystem::IsValid() const
{
	return LastErrorCode == 0;
}

const std::string& ps::FileSystem::GetName() const
{
	return Name;
}

std::unique_ptr<ps::FileSystem> ps::FileSystem::Open(const std::string& dir)
{
	std::error_code c;

	if (std::filesystem::exists(dir + "\\.build.info", c))
	{
		return std::make_unique<ps::CascFileSystem>(dir);
	}
	else
	{
		return std::make_unique<ps::WinFileSystem>(dir);
	}
}
