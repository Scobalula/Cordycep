#include "pch.h"
#include "Helper.h"
#include "GameModule.h"
#include "Parasyte.h"
#include "detours/detours.h"

// For manipulating PE Module
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

bool ps::GameModule::Load(std::string filePath)
{
    if (Handle != NULL)
    {
        // Free if already loaded
        FreeLibrary(Handle);
    }

    Handle = LoadLibraryA(filePath.c_str());

    if (Handle == NULL)
    {
        // TODO: Log
        Checksum = 0;
        Loaded = false;
        return false;
    }

    Loaded = true;
    Checksum = CalculateModuleCodeChecksum();

    return true;
}

bool ps::GameModule::Free()
{
    if (Handle != NULL)
    {
        FreeLibrary(Handle);
        return true;
    }

    return false;
}

uint64_t ps::GameModule::CalculateModuleCodeChecksum()
{
    auto header = ImageNtHeader(Handle);
    auto section = IMAGE_FIRST_SECTION(header);
    auto textBeg = (uint8_t*)Handle + section->VirtualAddress;

    return XXHash64::hash(textBeg, (size_t)section->Misc.VirtualSize, 0);
}

char* ps::GameModule::GetCachedOffset(uint32_t id)
{
    // TODO: Implement some form of caching to avoid constant lookups
    return nullptr;
}

char* ps::GameModule::Resolve(char* result, ScanType type)
{
    if (result == nullptr)
        return nullptr;

    switch (type)
    {
        case ScanType::NoResolving:
        {
            return result;
        }
        case ScanType::FromEndOfData:
        {
            auto to = *(int32_t*)result;
            auto from = result + 4;
            return from + to;
        }
        case ScanType::FromEndOfByteCmp:
        {
            auto to = *(int32_t*)result;
            auto from = result + 5;
            return from + to;
        }
        case ScanType::FromModuleBegin:
        {
            auto from = *(int32_t*)result;
            return (char*)Handle + from;
        }
    }

    return nullptr;
}

char* ps::GameModule::FindVariableAddress(const Pattern& pattern, size_t offsetToData, std::string name, ScanType type)
{
    ps::log::Log(ps::LogType::Verbose, "Attempting to locate: %s", name.c_str());

    auto scan = Scan(pattern, false);

    if (scan.empty()) 
    {
        ps::log::Log(ps::LogType::Normal, "Failed to locate: %s", name.c_str());
        return nullptr;
    }

    char* resolved = Resolve(scan[0] + offsetToData, type);

    if (resolved == nullptr)
    {
        ps::log::Log(ps::LogType::Normal, "Failed to locate: %s", name.c_str());
        return nullptr;
    }

    ps::log::Log(ps::LogType::Verbose, "Successfully located: %s", name.c_str());
    return resolved;
}

bool ps::GameModule::FindVariableAddress(void* variable, const Pattern& pattern, size_t offsetToData, std::string name, ScanType type)
{
    *(uint64_t*)variable = (uint64_t)FindVariableAddress(pattern, offsetToData, name, type);
    return *(uint64_t*)variable != 0;
}

bool ps::GameModule::NullifyFunction(const Pattern& pattern, size_t offsetFromSig, std::string name, bool multiple, bool relative)
{
    ps::log::Log(ps::LogType::Verbose, "Attempting to locate: %s", name.c_str());

    auto scan = Scan(pattern, multiple);

    if (scan.empty())
    {
        throw std::exception("Failed to locate");
        return false;
    }

    for (auto& ptr : scan)
    {
        auto toNull = ptr;

        if (relative)
        {
            auto data = ptr + offsetFromSig;
            auto to = *(int32_t*)data;
            auto from = data + 4;

            toNull = from + to;
        }
        else
        {
            toNull += offsetFromSig;
        }

        DWORD d = 0;
        VirtualProtect((LPVOID)toNull, sizeof(uint8_t), PAGE_EXECUTE_READWRITE, &d);
        *(uint8_t*)toNull = 0xC3;
        VirtualProtect((LPVOID)toNull, sizeof(uint8_t), d, &d);
        FlushInstructionCache(GetCurrentProcess(), (LPVOID)toNull, sizeof(uint8_t));
    }

    ps::log::Log(ps::LogType::Verbose, "Successfully located: %s", name.c_str());
    return true;
}

bool ps::GameModule::PatchBytes(const Pattern& pattern, size_t offsetFromSig, std::string name, PBYTE data, size_t dataLen, bool multiple, bool relative)
{
    ps::log::Log(ps::LogType::Verbose, "Attempting to locate: %s", name.c_str());

    auto scan = Scan(pattern, multiple);

    if (scan.empty())
    {
        ps::log::Log(ps::LogType::Normal, "Failed to locate: %s", name.c_str());
        return false;
    }

    for (auto& ptr : scan)
    {
        auto toPatch = ptr;

        if (relative)
        {
            auto data = ptr + offsetFromSig;
            auto to = *(int32_t*)data;
            auto from = data + 4;

            toPatch = from + to;
        }
        else
        {
            toPatch += offsetFromSig;
        }

        DWORD d = 0;
        VirtualProtect((LPVOID)toPatch, dataLen, PAGE_EXECUTE_READWRITE, &d);
        std::memcpy(toPatch, data, dataLen);
        VirtualProtect((LPVOID)toPatch, dataLen, d, &d);
        FlushInstructionCache(GetCurrentProcess(), (LPVOID)toPatch, sizeof(uint8_t));
    }

    ps::log::Log(ps::LogType::Verbose, "Successfully located: %s", name.c_str());
    return true;
}

bool ps::GameModule::CreateDetour(uintptr_t source, uintptr_t destination)
{
    if (source == NULL)
        return false;
    if (destination == NULL)
        return false;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)source, (PVOID)destination);

    auto error = DetourTransactionCommit();

    if (error != NO_ERROR)
    {
        // TODO: Log a failed detour.
        return false;
    }

    return true;
}

bool ps::GameModule::CreateDetourEx(uintptr_t* source, uintptr_t destination)
{
    if (source == NULL)
        return false;
    if (destination == NULL)
        return false;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)*source, (PVOID)destination);

    auto error = DetourTransactionCommit();

    if (error != NO_ERROR)
    {
        // TODO: Log a failed detour.
        return false;
    }

    return true;
}

std::vector<char*> ps::GameModule::Scan(const Pattern& pattern, bool multiple)
{
    // size_t index = 0;

    auto header = ImageNtHeader(Handle);
    auto section = IMAGE_FIRST_SECTION(header);

    auto textBeg = (uint8_t*)Handle + section->VirtualAddress;
    auto textEnd = textBeg + section->Misc.VirtualSize;

    std::vector<char*> results;

    // Attempt to locate in cache first
    auto cached = CachedOffsets.find(pattern.PatternID);

    if (cached != CachedOffsets.end())
    {
        for (auto& offset : cached->second)
        {
            results.push_back((char*)(textBeg + offset));
        }
    }
    else
    {
        for (uint8_t* p = textBeg; p < textEnd; p++)
        {
            auto t = p;
            bool match = true;

            // Run a check across this byte range.
            for (size_t i = 0; i < pattern.Size; i++)
            {
                // Masked Unkown Value
                if (pattern.Mask[i] == 0x00)
                {
                    t++;
                    continue;
                }
                // End of Module
                if (t >= textEnd)
                {
                    match = false;
                    break;
                }
                // Doesn't match
                if (*t++ != pattern.Needle[i])
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                results.push_back((char*)(p));

                if (!multiple)
                    break;
            }
        }

        // Check these offsets
        for (auto& result : results)
        {
            CachedOffsets[pattern.PatternID].push_back((uintptr_t)((uint64_t)result - (uint64_t)textBeg));
        }
    }

    return results;
}

const void ps::GameModule::LoadCache(const std::string& cachePath)
{
    CachedOffsets.clear();

    std::ifstream cacheStream(cachePath, std::ios::binary);

    if (!cacheStream)
        return;

    uint64_t checksum = 0;

    if (!cacheStream.read((char*)&checksum, sizeof(checksum)))
        return;
    if (checksum != Checksum)
        return;

    uint64_t patternID = 0;
    uintptr_t cachedOffset = 0;

    while (true)
    {
        if (!cacheStream.read((char*)&patternID, sizeof(patternID)))
            return;
        if (!cacheStream.read((char*)&cachedOffset, sizeof(cachedOffset)))
            return;

        CachedOffsets[patternID].push_back(cachedOffset);
    }
}

const void ps::GameModule::SaveCache(const std::string& cachePath) const
{
    std::ofstream cacheStream(cachePath, std::ios::binary);

    if (!cacheStream)
        return;

    cacheStream.write((const char*)&Checksum, sizeof(Checksum));

    for (auto& cachedOffset : CachedOffsets)
    {
        for (auto& element : cachedOffset.second)
        {
            auto patternID = cachedOffset.first;
            auto cachedOffset = element;

            cacheStream.write((const char*)&patternID, sizeof(patternID));
            cacheStream.write((const char*)&cachedOffset, sizeof(cachedOffset));
        }
    }
}
