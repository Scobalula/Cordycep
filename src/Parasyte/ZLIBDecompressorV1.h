#pragma once
#include "zlib.h"
#include "Decompressor.h"

namespace ps
{
	// A class to handle ZLIB V1 Fast Files.
	class ZLIBDecompressorV1 : public Decompressor
	{
	private:
		// The distance to the next hash block.
		size_t DistanceToNextHashBlock;
		// The zlib stream.
		z_stream ZStream{};
	public:
		// Creates a new ZLIB V1 decompressor.
		ZLIBDecompressorV1(ps::FileHandle& file, bool secure);

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
		// Reads decompressed data from the data stream.
		size_t Read(void* ptr, const size_t size, const size_t offset) override;

		// Reads uncompressed data from the data stream.
		size_t ReadUncompressed(uint8_t *buffer, size_t size) const;
	};
}

