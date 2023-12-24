#pragma once

namespace ps
{
	namespace oodle
	{
		// Initializes the Oodle routines and modules.
		bool Initialize(const std::string& fileName);
		// Decompresses Oodle Data.
		size_t Decompress(uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize);
		// Clears the loaded Oodle routines and modules.
		bool Clear();
	}
}

