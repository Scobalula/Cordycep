#include "pch.h"
#include "XAsset.h"
#include "Memory.h"

ps::XAsset::XAsset(const size_t type, const size_t headerSize, ps::Memory* headerMemory, FastFile* owner, uint8_t* header) :
	Type(type),
	HeaderMemory(headerMemory),
	Header(nullptr),
	FirstChild(nullptr),
	LastChild(nullptr),
	ExtendedDataSize(0),
	ExtendedDataPtrOffset(0),
	HeaderSize(headerSize),
	Owner(owner),
	Previous(nullptr),
	Next(nullptr),
	ID(0),
	Temp(false)
{
	if (owner == nullptr)
		return;

	Header = (uint8_t*)HeaderMemory->Allocate(headerSize);

	if (header != nullptr)
		std::memcpy(Header, header, headerSize);
}

ps::XAsset::~XAsset()
{
	HeaderMemory->Deallocate(Header);
}

void ps::XAsset::Override(FastFile* owner, uint8_t* header, bool temp)
{
	auto newEntry = new XAsset(Type, HeaderSize, HeaderMemory, Owner, Header);

	newEntry->ID = ID;
	newEntry->Previous = nullptr;
	newEntry->Temp = Temp;

	// Copy extended data
	if (ExtendedData != nullptr)
	{
		newEntry->ExtendedData = std::make_unique<uint8_t[]>(ExtendedDataSize);
		newEntry->ExtendedDataSize = ExtendedDataSize;
		std::memcpy(newEntry->ExtendedData.get(), ExtendedData.get(), ExtendedDataSize);
		*(uint64_t*)(newEntry->Header + ExtendedDataPtrOffset) = (uint64_t)newEntry->ExtendedData.get();
	}

	if (LastChild == nullptr)
	{
		FirstChild = newEntry;
		LastChild = newEntry;
	}
	else
	{
		LastChild->Next = newEntry;
		newEntry->Previous = LastChild;
		LastChild = newEntry;
	}

	// Override our current header to maintain pointers
	// within fast files.
	std::memcpy(Header, header, HeaderSize);

	Owner = owner;
	Temp = temp;
	ExtendedData = nullptr;
	ExtendedDataSize = 0;
	ExtendedDataPtrOffset = 0;
}

void ps::XAsset::CopyTo(FastFile* owner, uint8_t* header, bool temp)
{
	// Override our current header to maintain pointers
	// within fast files.
	std::memcpy(Header, header, HeaderSize);

	Owner = owner;
	Temp = temp;
	ExtendedData = nullptr;
	ExtendedDataSize = 0;
	ExtendedDataPtrOffset = 0;
}

void ps::XAsset::AppendChild(FastFile* owner, uint8_t* header, bool temp)
{
	auto newEntry = new XAsset(Type, HeaderSize, HeaderMemory, Owner, header);

	newEntry->ID = ID;
	newEntry->Previous = nullptr;
	newEntry->Temp = temp;
	newEntry->Owner = owner;

	if (LastChild == nullptr)
	{
		FirstChild = newEntry;
		LastChild = newEntry;
	}
	else
	{
		LastChild->Next = newEntry;
		newEntry->Previous = LastChild;
		LastChild = newEntry;
	}
}

void ps::XAsset::ClearChildrenBelongToFastFile(FastFile* fastFile)
{
	XAsset* link = FirstChild;

	while (link != nullptr)
	{
		auto currentLink = link;

		if (currentLink->Owner != nullptr && currentLink->Owner->ID == fastFile->ID)
		{
			if (currentLink->Next != nullptr)
			{
				currentLink->Next->Previous = currentLink->Previous;
			}
			else
			{
				LastChild = currentLink->Previous;
			}

			if (link->Previous != nullptr)
			{
				currentLink->Previous->Next = currentLink->Next;
			}
			else
			{
				FirstChild = currentLink->Next;
			}

			link = currentLink->Next;
			delete currentLink;
		}
		else
		{
			link = currentLink->Next;
		}
	}
}

const ps::XAsset* ps::XAsset::FindChildOfRootFastFile()
{
	for (auto p = FirstChild; p != nullptr; p = p->Next)
	{
		if (p->Owner->Parent != nullptr)
		{
			return p;
		}
	}

	return nullptr;
}

const ps::XAsset* ps::XAsset::FindChildOfFastFile(const FastFile* fastFile) const
{
	for (auto p = FirstChild; p != nullptr; p = p->Next)
	{
		if (p->Owner->Parent == fastFile)
		{
			return p;
		}
	}

	return nullptr;
}