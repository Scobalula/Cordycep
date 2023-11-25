#pragma once

namespace ps
{
	// A class to hold an xasset alias.
	class XAssetAlias
	{
	public:
		// The alias that can be used for more user friendly lookup.
		std::string Alias;
		// The actual name of the asset/s as they appear in the game files.
		std::string Name;
		// The files associated with this alias.
		std::vector<std::string> Files;

		// Creates a new XAsset Alias with the alias and name.
		XAssetAlias(const std::string& alias, const std::string& name) : Alias(alias), Name(name) {}
	};
}