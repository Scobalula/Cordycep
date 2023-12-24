#pragma once
#include "GamePattern.h"

namespace ps
{
	// A class to hold a game config.
	class GameConfig
	{
	public:
		// The flag this config is tied to. The first one is used if no flag is set.
		std::string Flag;
		// The name of the module for this config.
		std::string ModuleName;
		// The name of the cache for this config.
		std::string CacheName;
		// The name of the sub-directory within the game's directory where the files are stored.
		std::string FilesDirectory;
		// The name of the alises for this config.
		std::string AliasesName;
		// The common files within the game that are required for shared content.
		std::vector<std::string> CommonFiles;
		// The files the handler depends on.
		std::vector<std::string> Dependencies;
		// The patterns within this config for resolving data.
		std::vector<GamePattern> Patterns;

		// Creates a new config.
		GameConfig(const std::string& flag);

		// Loads a game configs json file.
		static bool LoadConfigsJson(const std::string& filePath, std::map<std::string, GameConfig>& configs);
		// Loads a game configs toml file.
		static bool LoadConfigsToml(const std::string& filePath, std::map<std::string, GameConfig>& configs);
	};
}

