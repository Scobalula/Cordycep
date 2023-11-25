#include "pch.h"
#include "OodleDecompressorV2.h"

ps::OodleDecompressorV2::OodleDecompressorV2(ps::FileHandle& file, bool secure) :
	Decompressor(file, secure),
	BlockIndex(0),
	NextHashBlockIndex(511)
{
}

size_t ps::OodleDecompressorV2::Decompress(uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize)
{
	return ps::oodle::Decompress(input, inputSize, output, outputSize);
}

size_t ps::OodleDecompressorV2::Read(void* ptr, const size_t size, const size_t offset)
{
	char* ptrInternal = (char*)ptr;
	size_t remaining = size;

	while (true)
	{
		auto cacheRemaining = DecompressedBufferSize - DecompressedBufferOffset;

		if (cacheRemaining > 0)
		{
			auto toCopy = min(cacheRemaining, remaining);
			std::memcpy(ptrInternal, &DecompressedBuffer[DecompressedBufferOffset], toCopy);

			ptrInternal += toCopy;
			DecompressedBufferOffset += toCopy;
			remaining -= toCopy;
		}

		if (remaining <= 0)
			break;

		if (BlockIndex == 0)
		{
			if (File.Read<uint32_t>() != 0x43574902)
				throw std::exception("Invalid auth magic number.");
			
			File.Seek(32768, SEEK_CUR);
			File.Read<uint32_t>();
			File.Seek(4, SEEK_CUR);
		}

		CompressedBufferSize = File.Read<uint32_t>();
		DecompressedBufferSize = File.Read<uint32_t>();
		Flags = File.Read<uint32_t>();
		DecompressedBufferOffset = 0;

		if (CompressedBufferSize == 0)
			break;

		auto alignedSize = ((size_t)CompressedBufferSize + 0x3) & 0xFFFFFFFFFFFFFFC;

		CompressedBuffer = std::make_unique<uint8_t[]>(alignedSize);
		DecompressedBuffer = std::make_unique<uint8_t[]>(DecompressedBufferSize);

		File.Read(&CompressedBuffer[0], alignedSize);

		auto result = Decompress(
			CompressedBuffer.get(),
			CompressedBufferSize,
			DecompressedBuffer.get(),
			DecompressedBufferSize);

		if (result != DecompressedBufferSize)
		{
			throw new std::exception();
		}

		BlockIndex++;

		if (BlockIndex == NextHashBlockIndex)
		{
			File.Seek(16384, FILE_CURRENT);
			NextHashBlockIndex += 512;
		}
	}

	return size;
}
