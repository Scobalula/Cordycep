#pragma once

namespace ps
{
	/// <summary>
	/// Log Channels to output information to.
	/// </summary>
	enum class LogType
	{
		/// <summary>
		/// Normal Output
		/// </summary>
		Normal = 1 << 1,

		/// <summary>
		/// Verbosity, more information
		/// </summary>
		Verbose = 1 << 2,

		/// <summary>
		/// Debug Channel
		/// </summary>
		Debug = 1 << 3,

		/// <summary>
		/// Error Channel
		/// </summary>
		Error = 1 << 4,
	};
}