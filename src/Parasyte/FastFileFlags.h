#pragma once
#include "BitFlags.h"

namespace ps
{
	// An enum that contains fast file flags.
	enum class FastFileFlags
	{
		// No flags.
		None = 0,
		// This is a common fast file.
		Common = 1 << 0,
		// Don't load children.
		NoChildren = 1 << 1,
	};
}