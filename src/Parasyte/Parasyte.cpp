#include "pch.h"
#include "Parasyte.h"
#include "Helper.h"
#include "Utility.h"

ps::Parasyte::Parasyte() : CurrentHandler(nullptr), CurrentFastFile(nullptr)
{
}

bool ps::Parasyte::TrySetGameHandler(const std::list<std::unique_ptr<ps::GameHandler>>& handlers, const std::string& param)
{
	//Log(ps::LogType::Normal, "Attempting to locate a handler for: %s", param.c_str());

	//for (auto& handler : handlers)
	//{
	//	if (handler->IsValid(param))
	//	{
	//		Log(ps::LogType::Normal, "Using handler: %s for: %s", handler->GetName().c_str(), param.c_str());
	//		CurrentHandler = handler.get();
	//		CurrentHandler->ClearFlags();
	//		return true;
	//	}
	//}

	//Log(ps::LogType::Normal, "Failed to locate a handler for: %s", param.c_str());
	return false;
}

ps::Parasyte& ps::Parasyte::Instance()
{
	static Parasyte instance;

	return instance;
}

ps::Registry& ps::Parasyte::GetRegistry()
{
	static Registry instance("SOFTWARE\\Scobalula\\Parasyte");

	return instance;
}

void ps::Parasyte::SetCurrentHandler(GameHandler* handler)
{
	Instance().CurrentHandler = handler;
}

ps::GameHandler* ps::Parasyte::GetCurrentHandler()
{
	return Instance().CurrentHandler;
}

void ps::Parasyte::SetCurrentFastFile(FastFile* fastFile)
{
	Instance().CurrentFastFile = fastFile;
}

ps::FastFile* ps::Parasyte::GetCurrentFastFile()
{
	return Instance().CurrentFastFile;
}

void ps::Parasyte::VerifyHandler(bool needsInit)
{
	if (ps::Parasyte::GetCurrentHandler() == nullptr)
	{
		throw std::exception("A handler has not been set, did you forget to call \"sethandler\"? Type: help to get some usage info.");
	}
	if (needsInit && !ps::Parasyte::GetCurrentHandler()->Initialized)
	{
		throw std::exception("The current handler has not been initialized, did you forget to call \"init\"? Type: help to get some usage info.");
	}
}

bool ps::Parasyte::LoadFile(const std::string& name, size_t index, size_t count, BitFlags<FastFileFlags> flags)
{
	ps::log::Print("MAIN", "Attempting to load: %s (%llu/%llu), please wait...", name.c_str(), index, count);

	try
	{
		if (!ps::Parasyte::GetCurrentHandler()->DoesFastFileExists(name))
		{
			ps::log::Print("ERROR", "The file: %s could not be found, check the name and the platform you installed it from for available downloads (i.e. MP/SP).", name.c_str());
			ps::log::Print("ERROR", "If an update was recently performed, you may need to launch the game to perform any fix ups to the file system.", name.c_str());
			return false;
		}
		if (ps::Parasyte::GetCurrentHandler()->IsFastFileLoaded(name))
		{
			ps::log::Print("ERROR", "The file: %s is already loaded.", name.c_str());
			return false;
		}
		if (!ps::Parasyte::GetCurrentHandler()->LoadFastFile(name, nullptr, flags))
		{
			ps::log::Print("ERROR", "The file: %s failed to load, the log file has more information on why this happened.", name.c_str());
			ps::log::Print("ERROR", "If the game recently received an update, you may need to re-dump the required data.", name.c_str());
			ps::Parasyte::GetCurrentHandler()->UnloadFastFile(name);
			ps::Parasyte::GetCurrentHandler()->CleanUp();
			return false;
		}
		else
		{
			ps::log::Print("MAIN", "The file: %s loaded successfully.", name.c_str());
			ps::Parasyte::GetCurrentHandler()->CleanUp();
			return true;
		}
	}
	catch (std::exception& ex)
	{
		ps::log::Print("ERROR", "An internal error has occured while loading the file: %s.", ex.what());
		ps::Parasyte::GetCurrentHandler()->UnloadFastFile(name);
		ps::Parasyte::GetCurrentHandler()->CleanUp();
		return false;
	}
}

void ps::Parasyte::PushTelemtry(const char* key, const char* value)
{
	PushTelemtry((std::string)key, (std::string)value);
}

void ps::Parasyte::PushTelemtry(const std::string& key, const std::string& value)
{
	auto& instance = Instance();

	instance.Telemtry.push_back(std::make_pair(key, value));
}

const std::string ps::Parasyte::GetTelemtry(const char* key)
{
	return GetTelemtry((std::string)key);
}

const std::string ps::Parasyte::GetTelemtry(const std::string& key)
{
	auto& instance = Instance();

	if (instance.Telemtry.size() > 0)
	{
		for (auto it = instance.Telemtry.end(); it != instance.Telemtry.begin(); it--)
		{
			if (it->first == key)
			{
				return it->second;
			}
		}
	}

	return "";
}