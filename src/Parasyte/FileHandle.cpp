#include "pch.h"
#include "FileHandle.h"

ps::FileHandle::FileHandle(HANDLE handle, ps::FileSystem* fileSystem)
{
    FileSystem = fileSystem;
    Handle = handle;
}

ps::FileHandle::~FileHandle()
{
    FileSystem->CloseFile(Handle);
}

const bool ps::FileHandle::IsValid() const
{
    return Handle != NULL && Handle != INVALID_HANDLE_VALUE;
}

std::unique_ptr<uint8_t[]> ps::FileHandle::Read(const size_t size)
{
    return FileSystem->Read(Handle, size);
}

size_t ps::FileHandle::Read(uint8_t* buffer, const size_t size)
{
    return Read(buffer, 0, size);
}

size_t ps::FileHandle::Read(uint8_t* buffer, const size_t offset, const size_t size)
{
    return FileSystem->Read(Handle, buffer, offset, size);
}

size_t ps::FileHandle::Write(const uint8_t* buffer, const size_t size)
{
    return Write(buffer, 0, size);
}

size_t ps::FileHandle::Write(const uint8_t* buffer, const size_t offset, const size_t size)
{
    return FileSystem->Write(Handle, buffer, offset, size);
}

size_t ps::FileHandle::Tell()
{
    return FileSystem->Tell(Handle);
}

size_t ps::FileHandle::Seek(size_t position, size_t direction)
{
    return FileSystem->Seek(Handle, position, direction);
}

size_t ps::FileHandle::Size()
{
    return FileSystem->Size(Handle);
}

void ps::FileHandle::Close()
{
    FileSystem->CloseFile(Handle);
}

HANDLE ps::FileHandle::GetHandle()
{
    return Handle;
}
