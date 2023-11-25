#include "pch.h"
#include "Registry.h"

ps::Registry::Registry(const std::string& path) : Path(path), Buffer(new char[UINT16_MAX])
{
}

std::string ps::Registry::GetString(const std::string& key, const std::string& defaultVal)
{
    PVOID buf = &Buffer[0];
    DWORD bufSize = UINT16_MAX;

    if (RegGetValueA(HKEY_CURRENT_USER, Path.c_str(), key.c_str(), RRF_RT_REG_SZ, nullptr, buf, &bufSize) != ERROR_SUCCESS)
    {
        RegSetKeyValueA(HKEY_CURRENT_USER, Path.c_str(), key.c_str(), REG_DWORD, defaultVal.data(), defaultVal.size());
        return defaultVal;
    }

    return &Buffer[0];
}

void ps::Registry::SetString(const std::string& key, const std::string& val)
{
    RegSetKeyValueA(HKEY_CURRENT_USER, Path.c_str(), key.c_str(), REG_SZ, val.data(), val.size());
}
