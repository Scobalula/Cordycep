#include "pch.h"
#include "XAssetPool.h"

ps::XAssetPool::XAssetPool()
{
	Root = nullptr;
	End = Root;
}

ps::XAssetPool::XAssetPool(const size_t elemSize, const size_t poolSize)
{
	HeaderMemory = std::make_unique<ps::Memory>(elemSize, poolSize);
	AssetMemory = std::make_unique<ps::Memory>(elemSize, poolSize);
	Root = new XAsset(0xFFFFFFFF, 0xFFFFFFFF, HeaderMemory.get(), nullptr, nullptr);
	LookupTable = std::make_unique<std::map<uint64_t, XAsset*>>();
	End = Root;
}

ps::XAssetPool::~XAssetPool()
{
	for (auto p = Root; p != nullptr; )
	{
		auto curr = p;
		p = curr->Next;
		delete curr;
	}
}

void ps::XAssetPool::Initialize(const size_t elemSize, const size_t poolSize)
{
	if (HeaderMemory == nullptr)
	{
		HeaderMemory = std::make_unique<ps::Memory>(elemSize, poolSize);
		Root         = new XAsset(0xFFFFFFFF, 0xFFFFFFFF, HeaderMemory.get(), nullptr, nullptr);
		LookupTable  = std::make_unique<std::map<uint64_t, XAsset*>>();
		End          = Root;
	}
}

ps::XAsset* ps::XAssetPool::CreateEntry(const char* name, const size_t len, const size_t type, const size_t headerSize, FastFile* owner, uint8_t* header, bool temp)
{
	auto entry = new XAsset(type, headerSize, HeaderMemory.get(), owner, header);

	entry->ID = XXHash64::hash(name, len, 0);
	entry->Previous = End;
	entry->Temp = temp;

	End->Next = entry;
	End = entry;

	LookupTable->operator[](entry->ID) = entry;

	return entry;
}

ps::XAsset* ps::XAssetPool::CreateEntry(const uint64_t hash, const size_t type, const size_t headerSize, FastFile* owner, uint8_t* header, bool temp)
{
	auto entry = new XAsset(type, headerSize, HeaderMemory.get(), owner, header);

	entry->ID = hash;
	entry->Previous = End;
	entry->Temp = temp;

	End->Next = entry;
	End = entry;

	LookupTable->operator[](entry->ID) = entry;

	return entry;
}

ps::XAsset* ps::XAssetPool::LinkXAssetEntry(const char* name, const size_t type, const size_t size, const bool temp, uint8_t* header, ps::FastFile* owner)
{

	// Our result
	auto result = FindXAssetEntry(name, strlen(name), type);

	// We need to check if we have an existing asset to override
	// If we have, we need to override or append, maintaining same pointer
	// to the address of the header so that if we unload this ff, etc.
	// the pointers from other assets are maintained
	if (result != nullptr)
	{
		if (temp)
		{
			result->AppendChild(
				owner,
				header,
				temp);
		}
		else
		{
			result->Override(
				owner,
				header,
				temp);
		}
	}
	else
	{
		result = CreateEntry(
			name,
			strlen(name),
			type,
			size,
			owner,
			header,
			temp);
	}

	return result;
}

ps::XAsset* ps::XAssetPool::FindXAssetEntry(const char* name, const size_t len, const size_t type) const
{
	ps::XAsset* result = nullptr;

	auto id = XXHash64::hash(name, len, 0);
	auto potentialEntry = LookupTable->find(id);

	if (potentialEntry != LookupTable->end())
	{
		result = potentialEntry->second;
	}

	return result;
}

ps::XAsset* ps::XAssetPool::FindXAssetEntry(const uint64_t hash, const size_t type) const
{
	ps::XAsset* result = nullptr;

	auto id = hash;
	auto potentialEntry = LookupTable->find(id);

	if (potentialEntry != LookupTable->end())
	{
		result = potentialEntry->second;
	}

	return result;
}

void ps::XAssetPool::ClearFastFileAssets(FastFile* fastFile)
{
	for (auto i = Root; i != nullptr; )
	{
		auto current = i;

		current->ClearChildrenBelongToFastFile(fastFile);

		if (current->Owner != nullptr && current->Owner->ID == fastFile->ID)
		{
			if (current->LastChild != nullptr)
			{
				auto toOverride = current->LastChild;

				current->LastChild = toOverride->Previous;

				if (current->LastChild == nullptr)
				{
					current->FirstChild = nullptr;
				}
				else
				{
					current->LastChild->Next = nullptr;
				}

				current->CopyTo(toOverride->Owner, toOverride->Header, toOverride->Temp);

				// Copy extended data
				if (toOverride->ExtendedData != nullptr)
				{
					current->ExtendedData = std::make_unique<uint8_t[]>(toOverride->ExtendedDataSize);
					current->ExtendedDataSize = toOverride->ExtendedDataSize;
					current->ExtendedDataPtrOffset = toOverride->ExtendedDataPtrOffset;

					std::memcpy(current->ExtendedData.get(), toOverride->ExtendedData.get(), toOverride->ExtendedDataSize);
					*(uint64_t*)(current->Header + current->ExtendedDataPtrOffset) = (uint64_t)current->ExtendedData.get();
				}

				delete toOverride;
				i = current->Next;
			}
			else
			{
				// We're at the end
				if (current->Next == nullptr)
				{
					current->Previous->Next = nullptr;
					End = current->Previous;
					i = nullptr;
				}
				else
				{
					current->Previous->Next = current->Next;
					current->Next->Previous = current->Previous;
					i = current->Next;
				}

				LookupTable->erase(current->ID);
				delete current;
			}
		}
		else
		{
			i = current->Next;
		}
	}
}

void ps::XAssetPool::EnumerateEntries(std::function<void(XAsset*)> func)
{
	if (Root != nullptr)
	{
		for (auto p = Root; ; p = p->Next)
		{
			if (p->Header != nullptr)
			{
				func(p);
			}
			if (p->Next == nullptr)
			{
				break;
			}
		}
	}
}
