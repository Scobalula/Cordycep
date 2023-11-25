#include "pch.h"
#include "XAssetPackageRef.h"

ps::XAssetPackageRef::XAssetPackageRef() : FileHandle(NULL)
{
}

ps::XAssetPackageRef::XAssetPackageRef(const std::string& fileName) : FileHandle(NULL)
{
	auto tempHandle = CreateFileA(
		fileName.c_str(),
		FILE_READ_DATA | FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (tempHandle != INVALID_HANDLE_VALUE)
	{
		FileHandle = tempHandle;
	}
}

ps::XAssetPackageRef::XAssetPackageRef(const HANDLE storageHandle, const std::string& fileName)
{
	//if (!CascOpenFile(storageHandle, fileName.c_str(), NULL, NULL, &FileHandle))
	//{
	//	FileHandle = NULL;
	//}
}

ps::XAssetPackageRef::XAssetPackageRef(HANDLE handle) : FileHandle(handle)
{
}

ps::XAssetPackageRef::~XAssetPackageRef()
{
	//if (FileHandle != NULL && FileHandle != INVALID_HANDLE_VALUE)
	//{
	//	if(!CascCloseFile(FileHandle))
	//		CloseHandle(FileHandle);
	//	FileHandle = NULL;
	//}
}
