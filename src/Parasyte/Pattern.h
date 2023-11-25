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

		Pattern();

		void Update(const std::string& sig)
		{
			PatternID = XXHash64::hash(sig.c_str(), sig.size(), 0);
			std::memset(Needle, 0, sizeof(Needle));
			std::memset(Mask, 0, sizeof(Mask));
			Size = 0;

			// mapping of ASCII characters to hex values
			const uint8_t hashmap[] =
			{
				0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			};

			for (size_t i = 0; i < sig.size() && Size < 64; )
			{
				// Skip whitespace
				if (sig[i] == ' ')
				{
					i++;
					continue;
				}

				auto idx0 = 0;
				auto idx1 = 0;
				auto maskVal = 0;

				if (sig[i + 0] != '?')
				{
					idx0 = ((uint8_t)sig[i + 0] & 0x1F) ^ 0x10;
					maskVal |= 0xF;
				}
				if (sig[i + 1] != '?')
				{
					idx1 = ((uint8_t)sig[i + 1] & 0x1F) ^ 0x10;
					maskVal <<= 4;
					maskVal |= 0xF;
				}

				Mask[Size] = (uint8_t)maskVal;
				Needle[Size++] = (uint8_t)(hashmap[idx0] << 4) | hashmap[idx1];
				i += 2;
			}
		}

		void Update(const char* sig)
		{
			size_t sizeOfSig = strlen(sig);

			PatternID = XXHash64::hash(sig, sizeOfSig, 0);
			std::memset(Needle, 0, sizeof(Needle));
			std::memset(Mask, 0, sizeof(Mask));
			Size = 0;

			// mapping of ASCII characters to hex values
			const uint8_t hashmap[] =
			{
				0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			};

			for (size_t i = 0; i < sizeOfSig && Size < 64; )
			{
				// Skip whitespace
				if (sig[i] == ' ')
				{
					i++;
					continue;
				}

				auto idx0 = 0;
				auto idx1 = 0;
				auto maskVal = 0;

				if (sig[i + 0] != '?')
				{
					idx0 = ((uint8_t)sig[i + 0] & 0x1F) ^ 0x10;
					maskVal |= 0xF;
				}
				if (sig[i + 1] != '?')
				{
					idx1 = ((uint8_t)sig[i + 1] & 0x1F) ^ 0x10;
					maskVal <<= 4;
					maskVal |= 0xF;
				}

				Mask[Size] = (uint8_t)maskVal;
				Needle[Size++] = (uint8_t)(hashmap[idx0] << 4) | hashmap[idx1];
				i += 2;
			}
		}
	};
}

