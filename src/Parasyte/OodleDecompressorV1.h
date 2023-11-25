#pragma once
#include "Oodle.h"
#include "Decompressor.h"

namespace ps
{
	// A class to handle V1 Oodle Fast Files.
	class OodleDecompressorV1 : public Decompressor
	{
	private:
		// The current block index.
		size_t DistanceToNextHashBlock;
	public:
		// Creates a new Oodle V2 decompressor.
		OodleDecompressorV1(ps::FileHandle& file, bool secure);

		// Reads secure data from the stream.
		size_t ReadSecure(uint8_t* buffer, const size_t size);
		// Reads secure data from the stream.
		template <class T>
		T ReadSecure()
		{
			T r{};
			ReadSecure((uint8_t*)&r, sizeof(r));
			return r;
		}
		// Decompress data.
		size_t Decompress(uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize);
		// Reads decompressed data from the data stream.
		size_t Read(void* ptr, const size_t size, const size_t offset);
	};
}

