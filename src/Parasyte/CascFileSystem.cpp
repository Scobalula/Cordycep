#include "pch.h"
#include "CascLib.h"
#include "CascFileSystem.h"
#include "Parasyte.h"

bool progress(    // Return 'true' to cancel the loading process
    void* PtrUserParam,                        // User-specific parameter passed to the callback
    LPCSTR szWork,                              // Text for the current activity (example: "Loading "ENCODING" file")
    LPCSTR szObject,                            // (optional) name of the object tied to the activity (example: index file name)
    DWORD CurrentValue,                         // (optional) current object being processed
    DWORD TotalValue                            // (optional) If non-zero, this is the total number of objects to process
    )
{
    return false;
}

ps::CascFileSystem::CascFileSystem(const std::string& directory)
{
    StorageHandle = NULL;
    LastErrorCode = 0;

    CASC_OPEN_STORAGE_ARGS args{};
    args.PfnProgressCallback = progress;

    if (!CascOpenStorageEx(directory.c_str(), &args, false, &StorageHandle))
    {
        LastErrorCode = GetCascError();
    }

    Name = "CascFileSystem";
}

ps::CascFileSystem::~CascFileSystem()
{
    for (const auto openHandle : OpenHandles)
    {
        CascCloseFile(openHandle);
    }

    OpenHandles.clear();
    CascCloseStorage(StorageHandle);
}

HANDLE ps::CascFileSystem::OpenFile(const std::string& fileName, const std::string& mode)
{
    // Casc only supports reading
    if (mode != "r")
    {
        LastErrorCode = 0x345300;
        return NULL;
    }

    HANDLE result = NULL;

    if (!CascOpenFile(StorageHandle, fileName.c_str(), CASC_LOCALE_NONE, CASC_OPEN_BY_NAME, &result))
    {
        LastErrorCode = GetCascError();
        result = NULL;
    }
    else
    {
        LastErrorCode = 0;
        OpenHandles.push_back(result);
    }

    return result;
}

void ps::CascFileSystem::CloseHandle(HANDLE handle)
{
    CascCloseFile(handle);
}

bool ps::CascFileSystem::Exists(const std::string& fileName)
{
    CASC_FIND_DATA findData{};
    HANDLE fileHandle = CascFindFirstFile(StorageHandle, fileName.c_str(), &findData, NULL);
    bool result = fileHandle != INVALID_HANDLE_VALUE && findData.bFileAvailable > 0;
    CascFindClose(fileHandle);
    return result;
}

size_t ps::CascFileSystem::Read(HANDLE handle, uint8_t* buffer, const size_t offset, const size_t size)
{
    DWORD sizeRead = 0;

    if (!CascReadFile(handle, buffer + offset, (DWORD)size, &sizeRead))
    {
        LastErrorCode = 0x345301;
    }
    else
    {
        LastErrorCode = 0;
    }

    return sizeRead;
}

size_t ps::CascFileSystem::Write(HANDLE handle, const uint8_t* buffer, const size_t offset, const size_t size)
{
    LastErrorCode = 0x345302;
    return 0;
}

size_t ps::CascFileSystem::Tell(HANDLE handle)
{
    ULONGLONG result = 0;

    if (CascSetFilePointer64(handle, 0, &result, FILE_CURRENT))
    {
        LastErrorCode = 0x345302;
    }
    else
    {
        LastErrorCode = 0;
    }

    return result;
}

size_t ps::CascFileSystem::Seek(HANDLE handle, size_t position, size_t direction)
{
    ULONGLONG result = 0;

    if (CascSetFilePointer64(handle, position, &result, (DWORD)direction))
    {
        LastErrorCode = 0x345303;
    }
    else
    {
        LastErrorCode = 0;
    }

    return result;
}

size_t ps::CascFileSystem::Size(HANDLE handle)
{
    if (handle == NULL)
    {
        LastErrorCode = 0x345303;
        return 0;
    }

    ULONGLONG result = 0;

    if (!CascGetFileSize64(handle, &result))
    {
        LastErrorCode = 0x345303;
        result = 0;
    }
    else
    {
        LastErrorCode = 0;
    }

    return result;
}

size_t ps::CascFileSystem::EnumerateFiles(const std::string& directory, const std::string& pattern, bool topDirectory, std::function<void(const std::string&, const size_t)> onFileFound)
{
    // size_t entriesConsumed = 0;
    HANDLE fileHandle;
    CASC_FIND_DATA findData;
    size_t results = 0;

    // New handle for searching
    fileHandle = CascFindFirstFile(StorageHandle, pattern.c_str(), &findData, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    // Get the last directory path if using topDirectory
    std::filesystem::path lastDirPath;
    if (topDirectory)
    {
        lastDirPath = std::filesystem::path(GetLastDirectoryName(directory));
        ps::log::Log(ps::LogType::Verbose, "CascFileSystem is searching last dir path: %s.", lastDirPath.c_str());
    }

    // Searching
    do
    {
        // Check file if available
        if (!findData.bFileAvailable)
            continue;

        // Skip if the parent dir of file is not match config
        if (topDirectory)
        {
            const auto parentPath = std::filesystem::path(findData.szFileName).parent_path();

            if (parentPath != lastDirPath)
                continue;
        }

        // Call back if we found it
        onFileFound(findData.szFileName, findData.FileSize);
        results++;
    } while (CascFindNextFile(fileHandle, &findData));

    // Close handle
    CascFindClose(fileHandle);

    return results;
}
