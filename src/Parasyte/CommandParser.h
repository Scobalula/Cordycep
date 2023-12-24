#pragma once

namespace ps
{
	// A class to handle parsing commands.
	class CommandParser
	{
	private:
		// The current index.
		int CurrentIndex;

	public:
		// Args from the command line.
		std::vector<std::string> Args;

		// Creates a new command parser.
		CommandParser() : CurrentIndex(0) {}

		// Gets the arg at the given index.
		std::string const& operator[](size_t index) const;
		// Parses commands from CLI.
		void Parse(const char** args, const int start, const int count);
		// Parses the commands from a string.
		void Parse(const char* cmd, const size_t len);
		// Parses the commands from a string.
		void Parse(const std::string& cmd);
		// Checks if we have commands left.
		bool HasCommands() const;
		// Requests the previous item from the command list.
		const std::string& Previous();
		// Requests the next item from the command list.
		const std::string& Next();
	};
}
