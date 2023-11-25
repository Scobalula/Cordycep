#pragma once
#include "Parasyte.h"
#include "CommandParser.h"
#include "Initializer.h"

namespace ps
{
	// A class to hold a command.
	class Command
	{
	private:
		// The name of the command.
		std::string Name;
		// The description of the command.
		std::string Description;
		// The explaination of the command.
		std::string Explaination;
		// The example of the command.
		std::string Example;
		// If this command is visible or not.
		bool Visible;
	public:
		// Creates a new command with the given name and handler.
		Command(const char* name, const char* desc, const char* explaination, const char* example, bool visible) :
			Name(name),
			Description(desc),
			Explaination(explaination),
			Example(example),
			Visible(visible) {}

		// Creates a new command with the given name and handler.
		Command(const std::string& name, const std::string& desc, const std::string& explaination, const std::string& example, bool visible) :
			Name(name),
			Description(desc),
			Explaination(explaination),
			Example(example),
			Visible(visible) {}

		// Gets the command description.
		const std::string& GetDescription() const
		{
			return Description;
		}
		// Gets the command explaination.
		const std::string& GetExplaination() const
		{
			return Explaination;
		}
		// Gets the command example.
		const std::string& GetExample() const
		{
			return Example;
		}
		// Gets the name of this command.
		const std::string& GetName() const
		{
			return Name;
		}
		// Gets if this command is visible.
		const bool IsVisible() const
		{
			return Visible;
		}
		// Checks if this command is valid.
		const bool IsValid(const std::string& command) const
		{
			return GetName() == command;
		}
		// Checks if this command is valid.
		const bool IsValid(const char* command) const
		{
			return GetName() == command;
		}
		// Executes the command
		virtual void Execute(ps::CommandParser& parser) const = 0;

		// Gets the list of handlers.
		static std::list<std::unique_ptr<Command>>& GetCommands();
		// Parses and executes the provided commands.
		static void Run(ps::CommandParser& parser);
	};
}