#pragma once
#include "pch.h"
#include "Parasyte.h"
#include <Windows.h>

namespace ps
{
    namespace printer
    {
        // Console Colors, used with Set/GetFore/BackgroundColor
        enum class Color
        {
            Black       = 0,
            DarkBlue    = 1,
            DarkGreen   = 2,
            DarkCyan    = 3,
            DarkRed     = 4,
            DarkMagenta = 5,
            DarkYellow  = 6,
            Gray        = 7,
            DarkGray    = 8,
            Blue        = 9,
            Green       = 10,
            Cyan        = 11,
            Red         = 12,
            Magenta     = 13,
            Yellow      = 14,
            White       = 15
        };
        // Private Internal Properties
        namespace
        {
            constexpr uint16_t ForegroundMask = 0xF;
            constexpr uint16_t BackgroundMask = 0xF0;
            constexpr uint16_t ColorMask = 0xFF;
        }
        // Gets the Foreground Color
        Color GetForegroundColor();
        // Gets the Background Color
        Color GetBackgroundColor();
        // Sets the Foreground Color
        void SetForegroundColor(Color color);
        // Sets the Background Color
        void SetBackgroundColor(Color color);
        // Resets the Foreground Color to White
        void ResetForegroundColor();
        // Resets the Background Color to Black
        void ResetBackgroundColor();
        // Resets the Foreground and Background Colors to White/Black respectively
        void ResetColor();
        // Outputs the message to the Console
        template<typename ... Args>
        void WriteHeader(const char* header, const char* value, Args ... args)
        {
            size_t headerSize = strlen(header);
            char buffer[1024]{};
            SetBackgroundColor(Color::DarkBlue);
            SetForegroundColor(Color::Gray);
            std::cout << "[";
            SetForegroundColor(Color::White);
            std::cout << header << std::setw(12 - headerSize);
            SetForegroundColor(Color::Gray);
            std::cout << "]";
            ResetBackgroundColor();
            ResetForegroundColor();
            size_t size = (size_t)std::snprintf(nullptr, 0, value, args ...) + 1;
            if (size >= sizeof(buffer))
                size = sizeof(buffer) - 1;
            std::snprintf(buffer, size, value, args ...);
            std::cout << ": " << buffer;
            std::cout.flush();
        }
        // Outputs the message to the Console
        template<typename ... Args>
        void WriteLineHeader(const char* header, const char* value, Args ... args)
        {
            size_t headerSize = strlen(header);
            char buffer[1024]{};
            SetBackgroundColor(Color::DarkBlue);
            SetForegroundColor(Color::Gray);
            std::cout << "[";
            SetForegroundColor(Color::White);
            std::cout << header << std::setw(12 - headerSize);
            SetForegroundColor(Color::Gray);
            std::cout << "]";
            ResetBackgroundColor();
            ResetForegroundColor();
            size_t size = (size_t)std::snprintf(nullptr, 0, value, args ...) + 1;
            if (size >= sizeof(buffer))
                size = sizeof(buffer) - 1;
            std::snprintf(buffer, size, value, args ...);
            std::cout << ": " << buffer << "\n";
            std::cout.flush();
        }
        // Outputs the message to the Console
        template<typename ... Args>
        void WriteErrorHeader(const char* header, const char* value, Args ... args)
        {
            size_t headerSize = strlen(header);
            char buffer[1024]{};
            SetBackgroundColor(Color::DarkBlue);
            SetForegroundColor(Color::Gray);
            std::cout << "[";
            SetForegroundColor(Color::White);
            std::cout << header << std::setw(12 - headerSize);
            SetForegroundColor(Color::Gray);
            std::cout << "]";
            SetForegroundColor(Color::White);
            SetBackgroundColor(Color::DarkRed);
            size_t size = (size_t)std::snprintf(nullptr, 0, value, args ...) + 1;
            if (size >= sizeof(buffer))
                size = sizeof(buffer) - 1;
            std::snprintf(buffer, size, value, args ...);
            std::cout << ": " << buffer;
            ResetBackgroundColor();
            ResetForegroundColor();
            std::cout << "\n";
            std::cout.flush();
        }
    }
}