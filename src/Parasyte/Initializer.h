#pragma once

namespace ps
{
	template <typename AT, typename T>
	class Initializer
	{
	public:
		Initializer(std::list<std::unique_ptr<AT>>& items)
		{
			items.push_back(std::make_unique<T>());
		}
	};
}

#define PS_CINIT(AT, T, A) namespace { ps::Initializer<AT, T> ___init___ = ps::Initializer<AT, T>(A); }