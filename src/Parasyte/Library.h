#pragma once

namespace ps
{
	// A class to hold a library.
	class Library
	{
	private:
		// A handle to the library.
		HMODULE Handle;

	public:
		// Creates a new library.
		Library(const char* lib);
		// Creates a new library.
		Library(const std::string& lib);

		template <typename ReturnValue, typename... Args>
		ReturnValue Call(const char* procname, Args... args)
		{
			FARPROC fp = GetProcAddress(Handle, procname);
			if (!fp)
				throw E_POINTER;
			typedef ReturnValue(__stdcall* function_pointer)(Args...);
			function_pointer P = (function_pointer)fp;
			const auto return_value = (*P)(std::forward<Args>(args)...);
			return return_value;
		}

		template <typename... Args>
		void Call(const char* procname, Args... args)
		{
			FARPROC fp = GetProcAddress(Handle, procname);
			if (!fp)
				throw E_POINTER;
			typedef void(__stdcall* function_pointer)(Args...);
			function_pointer P = (function_pointer)fp;
			(*P)(std::forward<Args>(args)...);
		}
	};
}

