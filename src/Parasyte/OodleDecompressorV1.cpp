#include "pch.h"
#include "OodleDecompressorV1.h"

ps::OodleDecompressorV1::OodleDecompressorV1(ps::FileHandle& file, bool secure) :
	Decompressor(file, secure),
	DistanceToNextHashBlock(SIZE_MAX)
{
}

size_t ps::OodleDecompressorV1::ReadSecure(uint8_t* buffer, const size_t size)
{
	if (DistanceToNextHashBlock == SIZE_MAX)
	{
		File.Seek(32768, SEEK_CUR);

		DistanceToNextHashBlock = 0x800000;

		if (File.Read<uint32_t>() != 0x43574902)
			throw std::exception("Failed to read authorization magic number.");

		File.Read<uint32_t>();
		File.Seek(4, SEEK_CUR);

		DistanceToNextHashBlock -= 12;
	}

	uint8_t* buffInternal = buffer;
	size_t dataConsumed = 0;

	if (DistanceToNextHashBlock < size)
	{
		File.Read(buffInternal, DistanceToNextHashBlock);
		File.Seek(0x4000, SEEK_CUR);
		dataConsumed += DistanceToNextHashBlock;
		buffInternal += DistanceToNextHashBlock;
		DistanceToNextHashBlock = 0x800000;
	}

	File.Read(buffInternal, 0, size - dataConsumed);
	DistanceToNextHashBlock -= size - dataConsumed;

	return size;
}

size_t ps::OodleDecompressorV1::Decompress(uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize)
{
	return ps::oodle::Decompress(input, inputSize, output, outputSize);
}

size_t ps::OodleDecompressorV1::Read(void* ptr, const size_t size, const size_t offset)
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

		CompressedBufferSize = ReadSecure<uint32_t>();
		DecompressedBufferSize = ReadSecure<uint32_t>();
		Flags = ReadSecure<uint32_t>();
		DecompressedBufferOffset = 0;

		auto alignedSize = ((size_t)CompressedBufferSize + 0x3) & 0xFFFFFFFFFFFFFFC;

		CompressedBuffer = std::make_unique<uint8_t[]>(alignedSize);
		DecompressedBuffer = std::make_unique<uint8_t[]>(DecompressedBufferSize);

		ReadSecure(&CompressedBuffer[0], alignedSize);

		auto result = Decompress(
			CompressedBuffer.get(),
			CompressedBufferSize,
			DecompressedBuffer.get(),
			DecompressedBufferSize);

		if (result != DecompressedBufferSize)
		{
			throw std::exception("Failed to decompress Oodle Data.");
		}
	}

	return size;
}
