#pragma once
#include "Initializer.h"
#include "GameModule.h"
#include "GameConfig.h"
#include "FastFile.h"
#include "XAsset.h"
#include "XAssetAlias.h"
#include "XAssetPool.h"
#include "FileSystem.h"

namespace ps
{
	// A base class for game handlers to inherit from.
	class GameHandler
	{
	public:
		// Gets the game id.
		uint64_t ID;
		// The native game module used to work with files.
		GameModule Module;
		// The loaded fast files.
		std::vector<std::unique_ptr<FastFile>> FastFiles;
		// Whether or not we are initialized.
		bool Initialized;
		// The number of asset pools.
		size_t XAssetPoolCount;
		// A linked list of assets for each asset pool.
		std::unique_ptr<XAssetPool[]> XAssetPools;
		// String lookup table.
		std::unique_ptr<std::map<uint64_t, size_t>> StringLookupTable;
		// Strings stored within the game
		std::unique_ptr<char[]> Strings;
		// The size of the above string pool.
		size_t StringPoolSize;
		// The game specific region prefix.
		std::string RegionPrefix;
		// The current game directory.
		std::string GameDirectory;
		// The xasset aliases.
		std::multimap<std::string, XAssetAlias> Aliases;
		// The current file system handler.
		std::unique_ptr<ps::FileSystem> FileSystem;
		// The game configs.
		std::map<std::string, GameConfig> Configs;
		// The current game config.
		GameConfig* CurrentConfig;
		// The game specific flags as a string array.
		std::vector<std::string> Flags;
		// The game variables resolved from patterns.
		std::map<std::string, void*> Variables;

		// Creates a new game handler.
		GameHandler(uint64_t id) :
            ID(id),
			Initialized(false),
			XAssetPoolCount(0),
			Strings(nullptr),
			StringPoolSize(0),
			CurrentConfig(nullptr)
		{
		}

		// Gets the name of the handler.
		virtual const std::string GetShorthand() = 0;
		// Gets the name of the handler.
		virtual const std::string GetName() = 0;
		// Gets the names of common files required to be loaded.
		virtual bool GetFiles(const std::string& pattern, std::vector<std::string>& results);
		// Gets the names of common files required to be loaded.
		virtual const std::vector<std::string>& GetCommonFileNames() const;
		// Initializes the handler, including loading any common fast files.
		virtual bool Initialize(const std::string& gameDirectory) = 0;
		// Opens the game directory using the correct file system.
		virtual void OpenGameDirectory(const std::string& gameDirectory);
		// Opens the game's module.
		virtual void OpenGameModule(const std::string& moduleName);
		// Deinitializes the handler, including unloading any fast files and closing file handles.
		virtual bool Deinitialize() = 0;
		// Checks if this handler is valid for the provided directory.
		virtual bool IsValid(const std::string& param) = 0;
		// Lists all supported files within the game.
		virtual bool LogFiles();
		// Lists loaded files.
		virtual void ListLoaded();
		// Generates a cache if supported by the title.
		virtual bool GenerateCache();
		// Generates a cache if supported by the title.
		virtual bool LoadCache();
		// Loads an alias list.
		virtual bool LoadAliases(const std::string& aliasesFile);
		// Sets the game specific region prefix.
		virtual void SetRegionPrefix(const std::string& regPrefix);
		// Checks if the file exists.
		virtual bool DoesFastFileExists(const std::string& name);
		// Checks if the file is already loaded.
		virtual bool IsFastFileLoaded(const std::string& ffName);
		// Loads the fast file with the given name, along with any children such as localized files.
		virtual bool LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags) = 0;
		// Dumps the aliases.
		virtual bool DumpAliases();
		// Cleans up any left over data after calling load.
		virtual bool CleanUp() = 0;
		// Unloads all fast files, along with any children such as localized files.
		virtual bool UnloadAllFastFiles();
		// Unloads all fast files, along with any children such as localized files.
		virtual bool UnloadNonCommonFastFiles();
		// Unloads the fast file, along with any children such as localized files.
		virtual bool UnloadFastFile(const std::string& ffName);
		// Creates a new unique fast file.
		virtual FastFile* CreateUniqueFastFile(const std::string& name);
		// Clears assets owned by the provided fast file.
		virtual void ClearFastFileAssets(FastFile* fastFile);
		// Adds a game specific flag to this handler.
		virtual void AddFlag(const std::string& flag);
		// Removes a game specific flag to this handler.
		virtual void RemoveFlag(const std::string& flag);
		// Removes all flags from this handler.
		virtual void ClearFlags();
		// Checks if the provided game specific flag is available.
		virtual bool HasFlag(const std::string& flag);
		// Gets a file path for the provided fast file name with the current flags and directory.
		virtual std::string GetFileName(const std::string& name);
		// Loads a game configs file.
		virtual bool LoadConfigs(const std::string& prefix);
		// Calculates the config if the required flag is set. If no flag is set, the first config is used.
		virtual bool SetConfig();
		// Resolves patterns within the game module.
		virtual bool ResolvePatterns();
		// Copies dependencies.
		virtual bool CopyDependencies();
		// Decrypt a string
		virtual char* DecryptString(char* str);

		// Gets the list of handlers.
		static std::list<std::unique_ptr<GameHandler>>& GetHandlers();
		// Finds a handler for the given param.
		static GameHandler* FindHandler(const std::string& param);
	};
}

// Sets a game variable resolved from patterns.
#define PS_SETGAMEVAR(VAR) { VAR = reinterpret_cast<decltype(VAR)>(Variables[#VAR]); }
// Detours a game variable resolved from patterns.
#define PS_DETGAMEVAR(VAR) { Module.CreateDetour((uintptr_t)Variables[#VAR], (uintptr_t)&VAR); }
// Intercepts and detours a game variable resolved from patterns.
#define PS_INTGAMEVAR(VAR, STORAGE) { STORAGE = reinterpret_cast<decltype(STORAGE)>(Variables[#VAR]); Module.CreateDetourEx((uintptr_t*)&STORAGE, (uintptr_t)&VAR); }
