#pragma once
#if _WIN64

namespace ps
{
	// A class to handle Call of Duty: Modern Warfare 2 Campaign Remastered Files
	class CoDMW2RHandler : public GameHandler
	{
	public:
		// Creates a new instance of this handler.
		CoDMW2RHandler() : GameHandler(0x32305453414D4552) {}

		// Gets the shorthand used for setting the handler.
		const std::string GetShorthand();
		// Gets the name of the handler.
		const std::string GetName();
		// Gets the names of common files required to be loaded.
		const bool GetFiles(const std::string& pattern, std::vector<std::string>& results);
		// Gets the names of common files required to be loaded.
		virtual const std::vector<std::string>& GetCommonFileNames() const;
		// Initializes the handler, including loading any common fast files.
		bool Initialize(const std::string& gameDirectory);
		// Uinitializes the handler, including deloading any fast files and closing file handles.
		bool Uninitialize();
		// Checks if this handler is valid for the provided directory.
		bool IsValid(const std::string& param);
		// Lists all supported files within the game.
		bool ListFiles();
		// Checks if the file exists.
		bool DoesFastFileExists(const std::string& ffName);
		// Loads the fast file with the given name, along with any children such as localized files.
		bool LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags);
		// Cleans up any left over data after calling load.
		bool CleanUp();
	};
}
#endif