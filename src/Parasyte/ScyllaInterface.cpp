#include "pch.h"
#include "ScyllaInterface.h"

ps::ScyllaInterface::ScyllaInterface()
{
	Module = NULL;
	ScyllaDumpProcessA = NULL;
}

ps::ScyllaInterface::~ScyllaInterface()
{
	FreeLibrary(Module);
}

const bool ps::ScyllaInterface::Initialize()
{
#if _WIN64
	Module = LoadLibraryA("Data\\Deps\\Scylla_x64.dll");
#else
	Module = LoadLibraryA("Data\\Deps\\Scylla_x86.dll");
#endif

	if (Module == NULL)
	{
		return false;
	}

	ScyllaDumpProcessA = (decltype(ScyllaDumpProcessA))GetProcAddress(Module, "ScyllaDumpProcessA");

	if (ScyllaDumpProcessA == NULL)
	{
		FreeLibrary(Module);
		return false;
	}

	return true;
}

const int ps::ScyllaInterface::Dump(const ps::ForeignProcess& foreignProcess, const std::string& path) const
{
	if (Module == NULL)
		return -1;
	if (ScyllaDumpProcessA == NULL)
		return -2;
	if (!ScyllaDumpProcessA(foreignProcess.GetID(), NULL, foreignProcess.GetMainModuleAddress(), foreignProcess.GetMainModuleAddress(), path.c_str()))
		return -3;


	return 0;
}
