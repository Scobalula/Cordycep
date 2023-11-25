#pragma once
#if _WIN64

namespace ps
{
	// A class to handle loading Call of Duty: Vanguard files.
	class CoDVGHandler : public GameHandler
	{
	public:
		// Creates a new handler for this title.
		CoDVGHandler() : GameHandler(0x44524155474E4156) {}

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
	};
}
#endif