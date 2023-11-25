#include "pch.h"
#include "Pattern.h"

ps::Pattern::Pattern()
{
    std::memset(Needle, 0, sizeof(Needle));
    std::memset(Mask, 0, sizeof(Mask));
    Size = 0;
}