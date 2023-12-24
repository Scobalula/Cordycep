#pragma once

namespace ps
{
	// A class to hold simple bit flags.
	template <class T>
	class BitFlags
	{
	private:
		// Underlying value.
		T Value;
	public:
		// Creates a new instance of the flags with blank flags.
		BitFlags() : Value((T)0) {}
		// Creates a new instance of the flags with provided flags.
		BitFlags(const T& val) : Value(val) {}

		// Adds the provided flag to this.
		T& AddFlag(const T& flag)
		{
			Value = (T)((size_t)Value | (size_t)flag);
			return *this;
		}
		// Removes the provided flag from this.
		T& RemoveFlag(const T& flag)
		{
			Value = (T)((size_t)Value ^ (size_t)flag);
			return *this;
		}
		// Checks if the provided flag is present in this.
		bool HasFlag(const T& flag) const
		{
			return ((size_t)Value & (size_t)flag) != 0;
		}
	};
}