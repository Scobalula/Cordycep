#include "pch.h"
#include "Utility.h"

ps::utility::ReadResult<uint8_t> ps::utility::ReadAllBytes(const std::string& fileName)
{
    ReadResult<uint8_t> result;
    std::ifstream s(fileName, std::ios::binary);

    if (s)
    {
        // Calculate the end position of the file, which
        // is effectively the size of the file.
        s.seekg(0, std::ios::end);
        size_t size = s.tellg();
        s.seekg(0, std::ios::beg);

        if (size > 0)
        {
            result.Buffer = std::make_unique<uint8_t[]>(size);
            result.Size = size;

            if (!s.read((char*)result.Buffer.get(), size))
            {
                result.Buffer = nullptr;
                result.Size = 0;
            }
        }
    }

    return result;
}

void ps::utility::ForceWindowFocus(HWND hWnd)
{
    SwitchToThisWindow(hWnd, TRUE);
    BringWindowToTop(hWnd);
    SetForegroundWindow(hWnd);
}

HWND ps::utility::GetHwndForPid(UINT pid)
{
    HWND hwndNext = FindWindowEx(NULL, NULL, NULL, NULL);
    while (hwndNext)
    {
        DWORD pid2;
        GetWindowThreadProcessId(hwndNext, &pid2);
        if (pid == pid2)
            return hwndNext;
        else
            hwndNext = FindWindowEx(NULL, hwndNext, NULL, NULL);
    };
    return NULL;
}
