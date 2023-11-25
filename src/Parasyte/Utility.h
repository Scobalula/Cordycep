#pragma once

namespace ps
{
	namespace utility
	{
		// The results of read all bytes
		template <typename T>
		class ReadResult
		{
		public:
			// The size of the buffer read.
			size_t Size = 0;
			// The resulting buffer read.
			std::unique_ptr<T[]> Buffer = nullptr;
		};

		// Consumes all bytes from the provided file.
		ReadResult<uint8_t> ReadAllBytes(const std::string& fileName);
	}
}