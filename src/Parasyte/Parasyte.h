#pragma once
#include "GameHandler.h"
#include "GameModule.h"
#include "Registry.h"
#include "LogType.h"
#include "Log.h"

namespace ps
{
	// Parasyte's Main Context Class
	class Parasyte
	{
	public:
		// The log mutex.
		std::mutex LogMutex;
		// Telemtry.
		std::vector<std::pair<std::string, std::string>> Telemtry;
		// The current handler
		GameHandler* CurrentHandler;
		// The current/last fast file loaded.
		ps::FastFile* CurrentFastFile;

		// Initializes a new instance of Parasyte.
		Parasyte();
		// Attempts to get a game handler for the given param.
		bool TrySetGameHandler(const std::list<std::unique_ptr<ps::GameHandler>>& handlers, const std::string& param);
		// Returns the global Parasyte instance/singleton.
		static Parasyte& Instance();
		// Returns the global Parasyte registry instance/singleton.
		// static Registry& GetRegistry();
		// Sets the current handler.
		static void SetCurrentHandler(GameHandler* handler);
		// Gets the current handler.
		static GameHandler* GetCurrentHandler();
		// Sets the current fast file being loaded.
		static void SetCurrentFastFile(FastFile* fastFile);
		// Gets the current fast file being loaded.
		static FastFile* GetCurrentFastFile();
		// Verifies the current handler is actually set.
		static void VerifyHandler(bool needsInit = true);
		// Loads the provided prefix.
		static bool LoadFile(const std::string& name, size_t index, size_t count, BitFlags<FastFileFlags> flags);
		// Dumps the aliases.
		static bool DumpAliases();
		// Pushes telemtry.
		static void PushTelemtry(const char* key, const char* value);
		// Pushes telemtry.
		static void PushTelemtry(const std::string& key, const std::string& value);
		// Gets the last telemtry matching this item.
		static const std::string GetTelemtry(const char* key);
		// Gets the last telemtry matching this item.
		static const std::string GetTelemtry(const std::string& key);
	};
}