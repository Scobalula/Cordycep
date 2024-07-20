#include "pch.h"
#include "ZLIBDecompressorV1.h"

ps::ZLIBDecompressorV1::ZLIBDecompressorV1(ps::FileHandle& file, bool secure) :
	Decompressor(file, secure),
	DistanceToNextHashBlock(SIZE_MAX)
{
	CompressedBufferSize = 262144;
	CompressedBuffer = std::make_unique<uint8_t[]>(CompressedBufferSize);

	// TODO: Check the return value of inflateInit(), and make a Destructor function for inflateEnd
	if (inflateInit(&ZStream))
	{
		// printf("Ok");
	}
}

size_t ps::ZLIBDecompressorV1::ReadSecure(uint8_t* buffer, const size_t size)
{
	if (Secure)
	{
		if (DistanceToNextHashBlock == SIZE_MAX)
		{
			File.Seek(0x4000, SEEK_CUR);
			DistanceToNextHashBlock = 0x200000;
		}

		uint8_t* buffInternal = buffer;
		size_t dataConsumed = 0;

		if (DistanceToNextHashBlock < size)
		{
			File.Read(buffInternal, DistanceToNextHashBlock);
			File.Seek(0x2000, SEEK_CUR);
			dataConsumed += File.Read(buffInternal, DistanceToNextHashBlock);
			buffInternal += dataConsumed;
			DistanceToNextHashBlock = 0x200000;
		}

		dataConsumed += File.Read(buffInternal, 0, size - dataConsumed);
		DistanceToNextHashBlock -= dataConsumed;
		return dataConsumed;
	}
	else
	{
		return File.Read(buffer, 0, size);
	}
}

size_t ps::ZLIBDecompressorV1::Read(void* ptr, const size_t size, const size_t offset)
{
	ZStream.next_out = (uint8_t*)ptr;
	ZStream.avail_out = (uInt)size;
	ZStream.total_out = 0;

	while (true)
	{
		auto rVal = inflate(&ZStream, Z_NO_FLUSH);

		if (rVal == Z_STREAM_END)
			break;
		if (rVal != Z_BUF_ERROR && rVal != Z_OK)
			throw std::exception();
		if (ZStream.avail_out == 0)
			break;

		ZStream.avail_in = (uInt)ReadSecure(CompressedBuffer.get(), CompressedBufferSize);
		ZStream.next_in = CompressedBuffer.get();

		if (ZStream.avail_in == 0)
			break;
	}

	return ZStream.total_out;
}

size_t ps::ZLIBDecompressorV1::ReadUncompressed(uint8_t* buffer, const size_t size) const
{
	return File.Read(buffer, 0, size);
}
