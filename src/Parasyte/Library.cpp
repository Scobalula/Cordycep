#include "pch.h"
#include "Library.h"

ps::Library::Library(const char* lib)
{
	Handle = LoadLibrary(lib);
}

ps::Library::Library(const std::string& lib)
{
	Handle = LoadLibrary(lib.c_str());
}
