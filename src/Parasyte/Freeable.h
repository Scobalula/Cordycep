#pragma once

namespace ps
{
	// A class to hold a freeable native C object.
	template <typename T, void (*F)(T*)>
	class Freeable
	{
	public:
		T Value{};

		Freeable() {}

		~Freeable()
		{
			F(&Value);
		}
	};
}

