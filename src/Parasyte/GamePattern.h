#pragma once
#include "GamePatternType.h"
#include "GamePatternFlag.h"

namespace ps
{
	// A class to hold a pattern definition.
	class GamePattern
	{
	public:
		// The pattern as a string.
		std::string Signature;
		// The name of the pattern if required.
		std::string Name;
		// The pattern type.
		GamePatternType Type;
		// The pattern flags.
		GamePatternFlag Flags;
		// The offset from where the pattern is found for where the required data is.
		size_t Offset;

		// Creates a new blank game pattern.
		GamePattern();

		// Adds the provided flag to the game pattern.
		void AddFlag(const GamePatternFlag& flag);
		// Removes the provided flag from the game pattern.
		void RemoveFlag(const GamePatternFlag& flag);
		// Checks if the provided flag is present in the game pattern.
		bool HasFlag(const GamePatternFlag& flag) const;
	};
}
