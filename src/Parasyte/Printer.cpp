#include "pch.h"
#include "Printer.h"

// Gets the Foreground Color
ps::printer::Color ps::printer::GetForegroundColor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO lpBufferInfo;
    if (GetConsoleScreenBufferInfo(hConsole, &lpBufferInfo))
        return (Color)(lpBufferInfo.wAttributes & ForegroundMask);
    return Color::Black;
}

ps::printer::Color ps::printer::GetBackgroundColor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO lpBufferInfo;
    if (GetConsoleScreenBufferInfo(hConsole, &lpBufferInfo))
        return (Color)((lpBufferInfo.wAttributes & BackgroundMask) >> 4);
    return Color::Black;
}

void ps::printer::SetForegroundColor(Color color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    auto bgColor = (int)GetBackgroundColor();
    SetConsoleTextAttribute(hConsole, ((int)color & ForegroundMask) | (bgColor << 4));
}

void ps::printer::SetBackgroundColor(Color color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    auto foreColor = (int)GetForegroundColor();
    SetConsoleTextAttribute(hConsole, (((int)color << 4) & BackgroundMask) | foreColor);
}

void ps::printer::ResetForegroundColor()
{
    SetForegroundColor(Color::White);
}

void ps::printer::ResetBackgroundColor()
{
    SetBackgroundColor(Color::Black);
}

void ps::printer::ResetColor()
{
    SetBackgroundColor(Color::White);
    SetBackgroundColor(Color::Black);
}