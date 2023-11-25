#pragma once

namespace ps
{
	// Flags for game patterns.
	enum class GamePatternFlag
	{
		// No flags.
		None = 0,
		// The scan lands us where we need to be and no resolving needs to be done.
		NoResolving = 1 << 0,
		// The data we are reading is relative to the start of the module.
		ResolveFromModuleBegin = 1 << 1,
		// We're resolving from a compare that is 7 bytes long where the offset is 2 bytes from the instruction.
		ResolveFromCmpToAddress = 1 << 2,
		// We're resolving from data that is 4 bytes long where the offset is relative to after the data.
		ResolveFromEndOfData = 1 << 3,
		// We're resolving from data that is 4 bytes long where the offset is relative to a byte after the data.
		ResolveFromEndOfByteCmp = 1 << 4,
		// We're resolving multiple values.
		ResolveMultipleValues = 1 << 5,
	};
}

