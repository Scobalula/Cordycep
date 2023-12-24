#pragma once

namespace ps
{
	class Pattern
	{
	public:
		uint64_t PatternID;
		uint8_t Needle[256];
		uint8_t Mask[256];
		size_t Size;

	public:
		Pattern();

		void Update(const std::string& sig);

		void Update(const char* sig);
	};
}
