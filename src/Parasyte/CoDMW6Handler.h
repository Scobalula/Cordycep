#pragma once
#if _WIN64

namespace ps
{
	// A class to handle Call of Duty: Modern Warfare 3 (2023) Files
	class CoDMW6Handler : public GameHandler
	{
	public:
		// Creates a new handler for MW6.
		CoDMW6Handler() : GameHandler(0x4B4F4D41594D4159) {}

		// Gets the shorthand used for setting the handler.
		const std::string GetShorthand();
		// Gets the name of the handler.
		const std::string GetName();
		// Initializes the handler, including loading any common fast files.
		bool Initialize(const std::string& gameDirectory);
		// Uinitializes the handler, including deloading any fast files and closing file handles.
		bool Uninitialize();
		// Checks if this handler is valid for the provided directory.
		bool IsValid(const std::string& param);
		// Loads the fast file with the given name, along with any children such as localized files.
		bool LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags);
		// Cleans up any left over data after calling load.
		bool CleanUp();
		// Gets a file path for the provided fast file name with the current flags and directory.
		std::string GetFileName(const std::string& name);
	};
}
#endif