#ifndef PCH_H
#define PCH_H


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#define CASCLIB_NO_AUTO_LINK_LIBRARY

#include "xxhash64.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <Windows.h>
#include <winternl.h>
#include <sddl.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <Lmcons.h>
#include <stdio.h>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <stack>
#include <functional>
#include <set>
#include <array>

#endif
