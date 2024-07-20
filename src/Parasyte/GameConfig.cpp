#include "pch.h"
#include "GameConfig.h"
#include "nlohmann/json.hpp"
#include <toml++/toml.hpp>

// Supported flags within config files.
std::map<std::string, ps::GamePatternFlag> PatternFlags
{
	{ "NoResolving",					ps::GamePatternFlag::NoResolving },
	{ "ResolveFromModuleBegin",			ps::GamePatternFlag::ResolveFromModuleBegin },
	{ "ResolveFromCmpToAddress",		ps::GamePatternFlag::ResolveFromCmpToAddress },
	{ "ResolveFromEndOfData",			ps::GamePatternFlag::ResolveFromEndOfData },
	{ "ResolveFromEndOfByteCmp",		ps::GamePatternFlag::ResolveFromEndOfByteCmp },
	{ "ResolveMultipleValues",			ps::GamePatternFlag::ResolveMultipleValues },
};

// Supported types within config files.
std::map<std::string, ps::GamePatternType> PatternTypes
{
	{ "Null",							ps::GamePatternType::Null },
	{ "Variable",						ps::GamePatternType::Variable },
};

ps::GameConfig::GameConfig(const std::string& flag) : Flag(flag)
{
}

bool ps::GameConfig::LoadConfigsJson(const std::string& filePath, std::map<std::string, GameConfig>& configs)
{
	std::ifstream stream(filePath);

	if (!stream)
		return false;

	nlohmann::json j;

	stream >> j;

	for (auto& jEntry : j)
	{
		GameConfig config(jEntry["Flag"]);

		config.ModuleName = jEntry["ModuleName"];
		config.CacheName = jEntry["CacheName"];
		config.FilesDirectory = jEntry["FilesDirectory"];
		config.AliasesName = jEntry["AliasesName"];

		// Remove trailing slashes and backslashes of FilesDirectory
		auto& str = config.FilesDirectory;
		while (!str.empty() && (str.back() == '/' || str.back() == '\\'))
		{
			str.pop_back();
		}

		// Append some information we need.
		for (auto& file : jEntry["CommonFiles"])
			config.CommonFiles.push_back(file);
		for (auto& file : jEntry["Dependencies"])
			config.Dependencies.push_back(file);

		// Append patterns
		for (auto& pattern : jEntry["Patterns"])
		{
			GamePattern gamePattern;

			gamePattern.Signature = pattern["PatternSignature"];
			gamePattern.Name = pattern.value("PatternName", "Anon");
			gamePattern.Offset = pattern.value("Offset", (size_t)0);
			gamePattern.Type = ps::GamePatternType::Variable;

			if (pattern["PatternType"].is_string())
			{
				auto foundType = PatternTypes.find(pattern["PatternType"]);

				if (foundType != PatternTypes.end())
				{
					gamePattern.Type = foundType->second;
				}
			}

			for (auto& flag : pattern["PatternFlags"])
			{
				auto foundFlag = PatternFlags.find(flag);

				if (foundFlag != PatternFlags.end())
				{
					gamePattern.AddFlag(foundFlag->second);
				}
			}

			config.Patterns.emplace_back(gamePattern);
		}

		configs.emplace(config.Flag, config);
	}

	return true;
}

bool ps::GameConfig::LoadConfigsToml(const std::string& filePath, std::map<std::string, GameConfig>& configs)
{
	// TODO: Check if the property exists
	try
	{
		// Parse the toml file
		toml::table tbl = toml::parse_file(filePath);

		// Debug
		// std::cout << tbl << "\n";

		// Pass in flag to initialize the config
		GameConfig config(*tbl["Flag"].value<std::string>());

		config.ModuleName = *tbl["ModuleName"].value<std::string>();
		config.CacheName = *tbl["CacheName"].value<std::string>();
		config.FilesDirectory = *tbl["FilesDirectory"].value<std::string>();
		config.AliasesName = *tbl["AliasesName"].value<std::string>();

		// Remove trailing slashes and backslashes of FilesDirectory
		auto& str = config.FilesDirectory;
		while (!str.empty() && (str.back() == '/' || str.back() == '\\'))
		{
			str.pop_back();
		}

		// Append some information we need.
		if (const auto commonFiles = tbl.get_as<toml::array>("CommonFiles"))
		commonFiles->for_each([&config](auto&& file)
		{
			// std::cout << file.value_or(std::string("None")) << "\n";
			config.CommonFiles.push_back(file.value_or(std::string("None")));
		});

		if (const auto deps = tbl.get_as<toml::array>("Dependencies"))
		deps->for_each([&config](auto&& file)
		{
			// std::cout << file.value_or(std::string("None")) << "\n";
			config.Dependencies.push_back(file.value_or(std::string("None")));
		});

		// Append patterns
		if (const auto patterns = tbl.get_as<toml::array>("Patterns"))
		patterns->for_each([&config](toml::table& pattern)
		{
			GamePattern gamePattern;

			gamePattern.Signature = pattern["PatternSignature"].value_or(std::string("NoPatternSignature"));
			gamePattern.Name = pattern["PatternName"].value_or(std::string("NoPatternName"));
			gamePattern.Offset = pattern["Offset"].value_or((size_t)0);
	
			// Pattern type
			const auto foundType = PatternTypes.find(pattern["PatternType"].value_or(std::string("NoPatternType")));
			if (foundType != PatternTypes.end())
			{
				gamePattern.Type = foundType->second;
			}
			else
			{
				// TODO: Log error
				// The corresponding type could not be found, use the default type
				gamePattern.Type = ps::GamePatternType::Variable;
			}
	
			// Pattern flags
			pattern.get_as<toml::array>("PatternFlags")->for_each([&gamePattern](auto&& flag)
			{
				auto foundFlag = PatternFlags.find(flag.value_or(std::string("NoPatternFlag")));
	
				if (foundFlag != PatternFlags.end())
				{
					gamePattern.AddFlag(foundFlag->second);
				}
				else
				{
					// TODO: Log error
					// The corresponding flag could not be found, nothing will be added
				}
			});
	
			config.Patterns.emplace_back(gamePattern);
		});

		configs.emplace(config.Flag, config);
	}
	catch (const toml::parse_error& err)
	{
		// Failed to parse the toml file
		// TODO: Use our method to log error
		std::cerr
		<< "Parsing failed: '" << *err.source().path
		<< "':\n" << err.description()
		<< "\n (" << err.source().begin << ")\n";

		return false;
	}

	// No exceptions were caught
	return true;
}