#include "pch.h"
#include "GamePattern.h"

ps::GamePattern::GamePattern() :
    Type(ps::GamePatternType::Variable),
    Flags(ps::GamePatternFlag::None),
    Offset(0)
{
}

void ps::GamePattern::AddFlag(const GamePatternFlag& flag)
{
    Flags = (GamePatternFlag)((int)Flags | (int)flag);
}

void ps::GamePattern::RemoveFlag(const GamePatternFlag& flag)
{
    Flags = (GamePatternFlag)((int)Flags ^ (int)flag);
}

const bool ps::GamePattern::HasFlag(const GamePatternFlag& flag) const
{
    return ((int)Flags & (int)flag) != 0;
}
