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

		// Force a window into focus
		// https://github.com/kweatherman/Folcolor/blob/33ecf0251263da04853400a1a5f47b4e957cf421/src/Controller/Utility.cpp#L71
		void ForceWindowFocus(HWND hWnd);

		// Get a PID's first related HWND if it has one.
		// https://github.com/kweatherman/Folcolor/blob/33ecf0251263da04853400a1a5f47b4e957cf421/src/Controller/Utility.cpp#L80
		HWND GetHwndForPid(UINT pid);
	}
}