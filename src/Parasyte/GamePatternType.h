#pragma once

namespace ps
{
	// A enum that defines supported pattern types.
	enum class GamePatternType
	{
		// Unknown.
		Unknown = 0,
		// A generic variable that points to an address.
		Variable,
		// A pattern that defines data that must be nulled.
		Null,
	};
}

