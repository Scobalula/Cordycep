#include "pch.h"
#include "Decompressor.h"

ps::Decompressor::Decompressor(ps::FileHandle& file, bool secure) :
	File(file),
	CompressedBufferSize(0),
	DecompressedBufferSize(0),
	DecompressedBufferOffset(0),
	Flags(0),
	Secure(secure)
{
}

bool ps::Decompressor::IsValid() const
{
	return File.IsValid();
}