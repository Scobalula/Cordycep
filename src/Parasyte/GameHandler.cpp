#include "pch.h"
#include "Parasyte.h"
#include "GameHandler.h"
#include "FileSystem.h"
#include "nlohmann/json.hpp"

bool ps::GameHandler::GetFiles(const std::string& pattern, std::vector<std::string>& results)
{
	if (!FileSystem)
	{
		// TODO: Log error
		return false;
	}

	ps::log::Log(ps::LogType::Normal, "Game files sub-directory: \"%s\".", CurrentConfig->FilesDirectory.c_str());

	// Get all the files in the directory and convert them to the form we want
	FileSystem->EnumerateFiles(CurrentConfig->FilesDirectory, pattern + ".ff", true, [&results](const std::string& fileName, const size_t size)
	{
		// We just need the base name of file
		std::string fileStemName = std::filesystem::path(fileName).stem().string();

		// Store it away
		results.emplace_back(fileStemName);
	});

	return true;
}

const std::vector<std::string>& ps::GameHandler::GetCommonFileNames() const
{
	return CurrentConfig->CommonFiles;
}

void ps::GameHandler::OpenGameDirectory(const std::string& gameDirectory)
{
	FileSystem = ps::FileSystem::Open(GameDirectory);

	ps::log::Log(ps::LogType::Verbose, "Using: %s for file system.", FileSystem->GetName().c_str());

	// Now we can determine if the file system opened correctly.
	if (!FileSystem->IsValid())
	{
		ps::log::Log(ps::LogType::Error, "Failed to open game directory using the provided directory, have you provided the correct path?");
		ps::log::Log(ps::LogType::Error, "Here's the error code returned: %llu", FileSystem->GetLastError());
		ps::log::Log(ps::LogType::Error, "Make sure you've provided the directory path and not a file path.");
		ps::log::Log(ps::LogType::Error, "Also make sure you've provided the game's base path, and not a sub path.");
		ps::log::Log(ps::LogType::Error, R"(For CoD HQ, do not provide "_retail_" or "sp22", just the base folder, usually with the game's name.)");
		ps::log::Log(ps::LogType::Error, R"(For example, steam's path looks like: "F:\Call of Duty HQ".)");
		ps::log::Log(ps::LogType::Error, "Cordycep may also not have permissions, try running as Admin.");

		throw std::exception("Failed to open the game's directory, check the log file for more information.");
	}

	ps::log::Log(ps::LogType::Normal, "Successfully opened file system in directory: %s.", gameDirectory.c_str());
	ps::log::Log(ps::LogType::Normal, "Using file system: %s", FileSystem->GetName().c_str());
}

void ps::GameHandler::OpenGameModule(const std::string& moduleName)
{
	if (!Module.Load(moduleName))
	{
		ps::log::Log(ps::LogType::Error, "Failed to load the dump file: %s", moduleName.c_str());
		ps::log::Log(ps::LogType::Error, "Error code: 0x%llx", (size_t)GetLastError());

		throw std::exception("Failed to open the game's dump, check the log file for more information.");
	}
}

bool ps::GameHandler::LogFiles()
{
	std::vector<std::string> files;
	GetFiles("*", files);
	ps::log::Log(ps::LogType::Normal, "Listing files from: %s", GetName().c_str());

	for (auto& file : files)
	{
		ps::log::Log(ps::LogType::Normal, "File: %s", file.c_str());
	}

	ps::log::Log(ps::LogType::Normal, "Listed: %llu files.", files.size());

	return true;
}

void ps::GameHandler::ListLoaded()
{
	ps::log::Log(ps::LogType::Normal, "Listing all loaded files");

	size_t totalMemoryUsage = 0;

	for (const auto& fastFile : FastFiles)
	{
		ps::log::Log(ps::LogType::Normal, "Loaded File: %s", fastFile->Name.c_str());

		for (size_t i = 0; i < FastFileBlockCount; i++)
		{
			if (fastFile->MemoryBlocks[i].MemorySize > 0)
			{
				ps::log::Log(ps::LogType::Verbose,
					"Memory Block: %llu @ 0x%llx using 0x%llx bytes of memory.",
					i,
					(uint64_t)fastFile->MemoryBlocks[i].Memory,
					(uint64_t)fastFile->MemoryBlocks[i].MemorySize);

				totalMemoryUsage += fastFile->MemoryBlocks[i].MemorySize;
			}
		}
	}

	ps::log::Log(ps::LogType::Verbose, "Memory Usage: 0x%llx", totalMemoryUsage);
	ps::log::Log(ps::LogType::Normal, "Loaded files: %llu", FastFiles.size());
}

bool ps::GameHandler::GenerateCache()
{
	ps::log::Log(LogType::Error, "This handler currently does not support asset caches.");
	return false;
}

bool ps::GameHandler::LoadCache()
{
	ps::log::Log(LogType::Error, "This handler currently does not support asset caches.");
	return false;
}

bool ps::GameHandler::LoadAliases(const std::string& aliasesFile)
{
	if (aliasesFile.empty())
	{
		ps::log::Log(LogType::Error, "This handler currently doesn't support asset aliases.");
		return false;
	}

	std::ifstream stream(aliasesFile);

	if (!stream)
	{
		ps::log::Log(LogType::Error, "The current handler can't locate the corresponding asset aliases file.");
		return false;
	}

	nlohmann::json j;

	stream >> j;

	for (auto& jEntry : j)
	{
		XAssetAlias alias(jEntry["alias"].get<std::string>(), jEntry["name"].get<std::string>());

		// Append the fast files for this alias.
		for (auto& file : jEntry["fast_files"])
			alias.Files.push_back(file.get<std::string>());
		std::ranges::transform(alias.Alias, alias.Alias.begin(), toupper);

		// If we have nothing to add, don't bother
		if (alias.Files.empty())
			continue;
		Aliases.emplace(alias.Alias, alias);
	}

	ps::log::Log(LogType::Normal, "The current handler has successfully located the corresponding asset aliases file.");

	return true;
}

void ps::GameHandler::SetRegionPrefix(const std::string& regPrefix)
{
	RegionPrefix = regPrefix;
}

bool ps::GameHandler::DoesFastFileExists(const std::string& name)
{
	return FileSystem != nullptr && FileSystem->Exists(GetFileName(name + ".ff"));
}

bool ps::GameHandler::IsFastFileLoaded(const std::string& ffName)
{
	auto id = XXHash64::hash(ffName.c_str(), ffName.size(), 0);

    const bool isFound = std::ranges::any_of(FastFiles, [&](auto& ff) {
        return ff->ID == id;
    });

	return isFound;
}

bool ps::GameHandler::DumpAliases()
{
    // TODO: Log for unsupported handlers
	return false;
}

bool ps::GameHandler::UnloadAllFastFiles()
{
	for (std::vector<std::unique_ptr<FastFile>>::iterator ffIterator = FastFiles.begin(); ffIterator != FastFiles.end(); )
	{
		for (size_t i = 0; i < XAssetPoolCount; i++)
		{
			XAssetPools[i].ClearFastFileAssets(ffIterator->get());
		}

		ffIterator = FastFiles.erase(ffIterator);
		// ps::XAsset::XAssetMemory->FreeEmptyPools();
	}

	return true;
}

bool ps::GameHandler::UnloadNonCommonFastFiles()
{
	for (std::vector<std::unique_ptr<FastFile>>::iterator ffIterator = FastFiles.begin(); ffIterator != FastFiles.end(); )
	{
		if (!ffIterator->get()->Common)
		{
			for (size_t i = 0; i < XAssetPoolCount; i++)
			{
				XAssetPools[i].ClearFastFileAssets(ffIterator->get());
			}

			ffIterator = FastFiles.erase(ffIterator);
		}
		else
		{
			++ffIterator;
		}
	}

	return true;
}

bool ps::GameHandler::UnloadFastFile(const std::string& ffName)
{
	const auto id = XXHash64::hash(ffName.c_str(), ffName.size(), 0);
	auto found = false;

	for (const auto& ff : FastFiles)
	{
		if (ff->ID == id)
		{
			found = true;
			break;
		}
	}

	// Verify it's found
	if (!found)
		return false;

	// First, remove children
	for (std::vector<std::unique_ptr<FastFile>>::iterator ffIterator = FastFiles.begin(); ffIterator != FastFiles.end(); )
	{
		if (ffIterator->get()->Parent != nullptr && ffIterator->get()->Parent->ID == id)
		{
			for (size_t i = 0; i < XAssetPoolCount; i++)
			{
				XAssetPools[i].ClearFastFileAssets(ffIterator->get());
			}

			ffIterator = FastFiles.erase(ffIterator);
			// ps::XAsset::XAssetMemory->FreeEmptyPools();
		}
		else
		{
			++ffIterator;
		}
	}

	for (std::vector<std::unique_ptr<FastFile>>::iterator ffIterator = FastFiles.begin(); ffIterator != FastFiles.end(); )
	{
		if (ffIterator->get()->ID == id)
		{
			for (size_t i = 0; i < XAssetPoolCount; i++)
			{
				XAssetPools[i].ClearFastFileAssets(ffIterator->get());
			}

			ffIterator = FastFiles.erase(ffIterator);
			// ps::XAsset::XAssetMemory->FreeEmptyPools();
		}
		else
		{
			++ffIterator;
		}
	}

	return true;
}

ps::FastFile* ps::GameHandler::CreateUniqueFastFile(const std::string& name)
{
	const auto id = XXHash64::hash(name.c_str(), name.size(), 0);

	// Ensure unique
	for (const auto& ff : FastFiles)
		if (ff->ID == id)
			throw std::exception("A fast file with the provided name is already loaded.");

	FastFiles.emplace_back(std::make_unique<FastFile>(name));
	return FastFiles.back().get();
}

void ps::GameHandler::ClearFastFileAssets(FastFile* fastFile)
{
	for (size_t i = 0; i < XAssetPoolCount; i++)
	{
		XAssetPools[i].ClearFastFileAssets(fastFile);
	}
}

void ps::GameHandler::AddFlag(const std::string& flag)
{
	if (HasFlag(flag))
		return;

	Flags.push_back(flag);
}

void ps::GameHandler::RemoveFlag(const std::string& flag)
{
	for (auto it = Flags.begin(); it != Flags.end(); )
	{
		if (flag.size() == it->size())
		{
			if (std::equal(flag.begin(), flag.end(), it->begin(), [](const char& a, const char b)
			{
				return a == b || tolower(a) == tolower(b);
			}))
			{
				it = Flags.erase(it);
			}
		}
	}
}

void ps::GameHandler::ClearFlags()
{
	Flags.clear();
}

bool ps::GameHandler::HasFlag(const std::string& flag)
{
	for (auto& setFlag : Flags)
	{
		if (flag.size() == setFlag.size())
		{
			if (std::equal(flag.begin(), flag.end(), setFlag.begin(), [](const char& a, const char b)
			{
				return a == b || tolower(a) == tolower(b);
			}))
			{
				return true;
			}
		}
	}
	return false;
}

std::string ps::GameHandler::GetFileName(const std::string& name)
{
	return name;
}

bool ps::GameHandler::LoadConfigs(const std::string& prefix)
{
	const std::string relativePath = "Data/Configs/";
	const std::string extension = ".toml";
	const auto configFolder = std::filesystem::current_path() / relativePath;
	auto _prefix = prefix;
	std::ranges::transform(_prefix, _prefix.begin(), tolower);
	bool canFoundConfig = false;

	for (const auto& entry : std::filesystem::directory_iterator(configFolder))
	{
		auto entry_filename = entry.path().filename().string();
		auto entry_ext = entry.path().extension().string();

		// Transform to lower
		std::ranges::transform(entry_filename, entry_filename.begin(), tolower);
		std::ranges::transform(entry_ext, entry_ext.begin(), tolower);

		// Load the config file if we found
		if (entry.is_regular_file() &&
			entry_filename.find(_prefix) == 0 &&
			entry_ext == extension)
		{
			GameConfig::LoadConfigsToml(entry.path().string(), Configs);
			ps::log::Log(ps::LogType::Normal, "Loaded config: %s", entry.path().filename().string().c_str());
			// ps::log::Print("MAIN", "Loaded config: %s", entry.path().filename().string().c_str());

			if (!canFoundConfig)
				canFoundConfig = true;
		}
	}

	// Failed to find any config;
	if (!canFoundConfig)
	{
		ps::log::Print("ERROR", "Failed to find any current game handler config in \"Data/Configs\".");

		return false;
	}

	return true;
}

bool ps::GameHandler::SetConfig()
{
	// Failed to find any config
	if (Configs.empty())
	{
		// TODO: Log 
		return false;
	}
	else // Use the config with the flag set when we have more than one config.
	{
		// TODO: Optimize
		if (Flags.empty())
		{
			for (auto& [flag, config] : Configs)
			{
				if (flag == "default")
				{
					CurrentConfig = &config;
					return true;
				}
			}
		}
		else
		{
			for (auto& [flag, config] : Configs)
			{
				if (HasFlag(flag))
				{
					CurrentConfig = &config;
					return true;
				}
			}
		}

		return false;
	}
}

bool ps::GameHandler::ResolvePatterns()
{
	if (CurrentConfig->Patterns.empty())
	{
		// TODO: Log
		return false;
	}

	Pattern patternBytes;
	bool success = true;

	for (auto& gamePattern : CurrentConfig->Patterns)
	{
		// First update our pattern and resolve the data.
		patternBytes.Update(gamePattern.Signature);
		
		auto addresses = Module.Scan(patternBytes, gamePattern.HasFlag(ps::GamePatternFlag::ResolveMultipleValues));

		if (addresses.empty())
		{
			ps::log::Log(ps::LogType::Error, "Failed to find pattern Name: %s, Signature: %s, Offset: %d", gamePattern.Name.c_str(), gamePattern.Signature.c_str(), gamePattern.Offset);
			success = false;	
		}
		else
		{
			for (const auto& address : addresses)
			{
				char* resolved = nullptr;

				if (gamePattern.HasFlag(ps::GamePatternFlag::NoResolving))
				{
					resolved = address + gamePattern.Offset;
				}
				else if (gamePattern.HasFlag(ps::GamePatternFlag::ResolveFromEndOfData))
				{
					const auto to = *(int32_t*)(address + gamePattern.Offset);
					const auto from = address + gamePattern.Offset + 4;
					resolved = from + to;
				}
				else if (gamePattern.HasFlag(ps::GamePatternFlag::ResolveFromEndOfByteCmp))
				{
					const auto to = *(int32_t*)(address + gamePattern.Offset);
					const auto from = address + gamePattern.Offset + 5;
					resolved = from + to;
				}
				else if (gamePattern.HasFlag(ps::GamePatternFlag::ResolveFromModuleBegin))
				{
					const auto from = *(int32_t*)(address + gamePattern.Offset);
					resolved = (char*)Module.Handle + from;
				}

				if (resolved == nullptr)
				{
					return false;
				}

				switch (gamePattern.Type)
				{
					case ps::GamePatternType::Null:
					{
						DWORD d = 0;
						VirtualProtect((LPVOID)resolved, sizeof(uint8_t), PAGE_EXECUTE_READWRITE, &d);
						*(uint8_t*)resolved = 0xC3;
						VirtualProtect((LPVOID)resolved, sizeof(uint8_t), d, &d);
						FlushInstructionCache(GetCurrentProcess(), (LPVOID)resolved, sizeof(uint8_t));
						break;
					}
					case ps::GamePatternType::Variable:
					{
						Variables[gamePattern.Name] = resolved;
						break;
					}
					case GamePatternType::Unknown:
						break;
				}
			}
		}
	}

	if (success != true)
		throw std::exception("Failed to find one or more game patterns. The log file has more information this.");

	return success;
}

bool ps::GameHandler::CopyDependencies()
{
	const auto depsDir = "Data/Deps";

	for (auto& dep : CurrentConfig->Dependencies)
	{
		std::filesystem::path src = GameDirectory / (std::filesystem::path)dep;
		std::filesystem::path dst = depsDir / src.filename();

		if (!std::filesystem::exists(dst) && std::filesystem::exists(src))
			std::filesystem::copy(src, dst);
	}

	return true;
}

char* ps::GameHandler::DecryptString(char* str)
{
	return str; // no decryption by default
}

std::list<std::unique_ptr<ps::GameHandler>>& ps::GameHandler::GetHandlers()
{
	static std::list<std::unique_ptr<ps::GameHandler>> handlers;

	return handlers;
}

ps::GameHandler* ps::GameHandler::FindHandler(const std::string& param)
{
	for (const auto& handler : GetHandlers())
	{
		if (handler->IsValid(param))
		{
			return handler.get();
		}
	}

	return nullptr;
}
