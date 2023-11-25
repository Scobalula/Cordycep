#pragma once
#include "Memory.h"
#include "FastFile.h"

namespace ps
{
	// A class to hold a generic xasset.
	class XAsset
	{
	public:
		// The asset header.
		uint8_t* Header;
		// Whether or not this asset is a temp slot.
		size_t Temp;
		// The next xasset in the list.
		XAsset* Next;
		// The previous xasset in the list.
		XAsset* Previous;
		// The xasset ID.
		size_t ID;
		// The xasset type depending on the game it is from.
		size_t Type;
		// The xasset size.
		size_t HeaderSize;
		// The pointer that points to the extended data.
		size_t ExtendedDataPtrOffset;
		// The size of the extended data.
		size_t ExtendedDataSize;
		// First child asset we have overriden.
		XAsset* FirstChild;
		// Last child asset we have overriden.
		XAsset* LastChild;
		// The fast file that owns this asset.
		FastFile* Owner;
		// Header Memory.
		ps::Memory* HeaderMemory;
		// The extended data, if any.
		std::unique_ptr<uint8_t[]> ExtendedData;

		// Creates a new XAsset with the provided data.
		XAsset(const size_t type, const size_t headerSize, ps::Memory* headerMemory, FastFile* owner, uint8_t* header);
		// Destroys the XAsset
		~XAsset();

		// Overrides this asset with the provided information and stores this as a child of the new entry asset.
		void Override(FastFile* owner, uint8_t* header, bool temp);
		// Copies the header to this asset.
		void CopyTo(FastFile* owner, uint8_t* header, bool temp);
		// Stores this as a child of the current entry asset.
		void AppendChild(FastFile* owner, uint8_t* header, bool temp);
		// Clears the children that are belong to this fast file, if any.
		void ClearChildrenBelongToFastFile(FastFile* fastFile);
		// Gets the first non-temp child that is owned by a root file.
		const XAsset* FindChildOfRootFastFile();
		// Gets the first non-temp child that is owned by a root file.
		const XAsset* FindChildOfFastFile(const FastFile* fastFile) const;
	};
}
