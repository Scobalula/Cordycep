#include "pch.h"
#include "CommandParser.h"

std::string const& ps::CommandParser::operator[](size_t index) const
{
	if (index >= Args.size())
	{
		throw std::exception("You passed a command that expected arguments but provided none, type \"Help\" for more info.");
	}

	return Args[index];
}

void ps::CommandParser::Parse(const char** args, const int start, const int count)
{
	CurrentIndex = 0;
	Args.clear();

	for (int i = start; i < count; i++)
	{
		Args.emplace_back(args[i]);
	}
}

void ps::CommandParser::Parse(const char* cmd, const size_t len)
{
	CurrentIndex = 0;
	Args.clear();

	size_t i = 0;

	std::string_view view(cmd);

	while (i < len)
	{
		// Whitespace
		if (cmd[i] == ' ' || cmd[i] == '\t')
		{
			i++;
			continue;
		}

		// String literal
		if (cmd[i] == '"')
		{
			i++;
			size_t start = i;
			size_t end = start;

			while (i < len)
			{
				if (cmd[i] == '"')
				{
					i++;
					break;
				}

				i++;
				end++;
			}

			auto subView = view.substr(start, end - start);

			if (subView.find_first_not_of(' ') != std::string::npos)
			{
				Args.emplace_back(view.substr(start, end - start));
			}
		}
		else
		{
			size_t start = i;
			size_t end = start;

			while (i < len)
			{
				if (cmd[i] == ' ' || cmd[i] == '\t')
				{
					break;
				}

				i++;
				end++;
			}

			auto subView = view.substr(start, end - start);

			if (subView.size() > 0 && subView.find_first_not_of(' ') != std::string::npos)
			{
				Args.emplace_back(view.substr(start, end - start));
			}
		}
	}
}

void ps::CommandParser::Parse(const std::string& cmd)
{
	return Parse(cmd.c_str(), cmd.size());
}

const bool ps::CommandParser::HasCommands() const
{
	return CurrentIndex < Args.size();
}

const std::string& ps::CommandParser::Previous()
{
	if (CurrentIndex < 0 || CurrentIndex >= Args.size())
	{
		throw std::exception("You passed a command that expected arguments but provided none, type \"Help\" for more info.");
	}

	return Args[CurrentIndex--];
}

const std::string& ps::CommandParser::Next()
{
	if (CurrentIndex < 0 || CurrentIndex >= Args.size())
	{
		throw std::exception("You passed a command that expected arguments but provided none, type \"Help\" for more info.");
	}

	return Args[CurrentIndex++];
}
