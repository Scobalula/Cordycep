#pragma once
#include "FileHandle.h"

namespace ps
{
	// A class to hold a decompressor.
	class Decompressor
	{
	protected:
		// The current compressed buffer.
		std::unique_ptr<uint8_t[]> CompressedBuffer;
		// The current decompressed buffer.
		std::unique_ptr<uint8_t[]> DecompressedBuffer;
		// The current compressed buffer size.
		size_t CompressedBufferSize;
		// The current decompressed buffer size.
		size_t DecompressedBufferSize;
		// The current offset within the decompressed buffer.
		size_t DecompressedBufferOffset;
		// The current decompressor flags.
		size_t Flags;
		// If the current file stream is secure.
		bool Secure;
		// The file handle.
		ps::FileHandle& File;

	public:
		// Creates a new decompressor.
		Decompressor(ps::FileHandle& file, bool secure);

		// Reads decompressed data from the data stream.
		virtual size_t Read(void* ptr, const size_t size, const size_t offset) = 0;
		// Checks if the provided decompressor is valid.
		virtual const bool IsValid() const;
	};
}

