#pragma once
#if _WIN64

namespace ps
{
	// A class to handle Call of Duty: Modern Warfare (2019) Files
	class CoDMW4Handler : public GameHandler
	{
	public:
		// Creates a new handler for the given title.
		CoDMW4Handler() : GameHandler(0x3931524157444F4D) {}

		// Gets the shorthand used for setting the handler.
		const std::string GetShorthand() override;

		// Gets the name of the handler.
		const std::string GetName() override;

		// Initializes the handler, including loading any common fast files.
		bool Initialize(const std::string& gameDirectory) override;

		// Deinitializes the handler, including unloading any fast files and closing file handles.
		bool Deinitialize() override;

		// Checks if this handler is valid for the provided directory.
		bool IsValid(const std::string& param) override;

		// Loads the fast file with the given name, along with any children such as localized files.
		bool LoadFastFile(const std::string& ffName, FastFile* parent, BitFlags<FastFileFlags> flags) override;

		// Decrypt string
		char* DecryptString(char* str) override;

		// Cleans up any left over data after calling load.
		bool CleanUp() override;
	};
}
#endif