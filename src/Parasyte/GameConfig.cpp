#include "pch.h"
#include "GameConfig.h"
#include "nlohmann/json.hpp"

ps::GameConfig::GameConfig(const std::string& flag) : Flag(flag)
{
}
