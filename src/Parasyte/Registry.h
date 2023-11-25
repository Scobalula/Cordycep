#pragma once

namespace ps
{
	// A class to handle reading and writing from the registry.
	class Registry
	{
	private:
		// The path of the registry.
		std::string Path;
		// A buffer for reading from the registry.
		std::unique_ptr<char[]> Buffer;
	public:
		// Creates a new Registry instance.
		Registry(const std::string& path);

		// Gets the string stored at the given key.
		std::string GetString(const std::string& key, const std::string& defaultVal);
		// Sets the string at the given key.
		void SetString(const std::string& key, const std::string& val);
	};
}