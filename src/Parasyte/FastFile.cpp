#include "pch.h"
#include "FastFile.h"

ps::FastFile::FastFile(std::string name)
{
    Name = name;
    Parent = nullptr;
    Common = false;
    ID = XXHash64::hash(name.c_str(), name.length(), 0);
    std::memset(AssetList, 0, sizeof(AssetList));
}