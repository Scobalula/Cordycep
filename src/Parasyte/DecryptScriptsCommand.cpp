#include "pch.h"
#include "DecryptScriptCommand.h"
#include "Utility.h"
#include "FileSystem.h"
#include "FileHandle.h"

ps::DecryptScriptCommand::DecryptScriptCommand() : Command(
	"decryptscript",
	"Decrypt compiled GSC scripts.",
	"This command decrypts a gscobj dump using the loaded game decryption.",
	"decryptscript \"C:\\gscobj_dump\" \"C:\\gscobj_dump_decrypted\"",
	true)
{
}

void ps::DecryptScriptCommand::Execute(ps::CommandParser& parser) const
{
	ps::Parasyte::VerifyHandler();


	const std::string& gscsPath = parser.Next();
	const std::string& gscsOutputPath = parser.Next();

	auto* handler = ps::Parasyte::GetCurrentHandler();

	ps::log::Print("MAIN", "Decrypting: %s into %s...", gscsPath.c_str(), gscsOutputPath.c_str());

	auto fsi = ps::FileSystem::Open(gscsPath);
	auto fso = ps::FileSystem::Open(gscsOutputPath);

	std::filesystem::create_directories(gscsOutputPath);

	fsi->EnumerateFiles("", "*.gscc", false, [handler, &fsi, &fso, &gscsOutputPath](const std::string& fileName, const size_t size)
	{
		if (size <= sizeof(uint64_t))
		{
			return; // too small
		}

		ps::FileHandle ffHandle{ fsi->OpenFile(fileName, "r"), fsi.get() };

		if (!ffHandle.IsValid())
		{
			ps::log::Print("WARNING", "Error when reading", fileName.c_str());
			return;
		}


		auto buff{ ffHandle.ReadArray<uint8_t>(size) };

		ffHandle.Close();

		uint64_t magic{ *(uint64_t*)buff.get() };

		struct GSC_USEANIMTREE_ITEM
		{
			uint32_t num_address;
			uint32_t address;
		};

		struct GSC_ANIMTREE_ITEM
		{
			uint32_t num_address;
			uint32_t address_str1;
			uint32_t address_str2;
		};
		struct GSC_STRINGTABLE_ITEM
		{
			uint32_t string;
			uint8_t num_address;
			uint8_t type;
			uint16_t pad;
		};


		switch (magic)
		{
		case 0xa0d4353478a: // MWIII 8A
			break; // nothing to do, can be present due to a remnant in the mwiii fastfiles
		case 0x37000a0d43534780: // CW 37
		{
			struct GscObjCW37
			{
				uint8_t magic[8];
				uint32_t crc;
				uint32_t pad0c;
				uint64_t name;
				uint32_t includes_table;
				uint16_t string_count;
				uint16_t export_count;
				uint32_t cseg_offset;
				uint32_t string_table;
				uint16_t imports_count;
				uint16_t fixup_count;
				uint32_t devblock_string_table;
				uint32_t exports_tables;
				uint32_t imports_offset;
				uint16_t globalvar_count;
				uint16_t unk3a;
				uint32_t fixup_offset;
				uint32_t globalvar_offset;
				uint32_t file_size;
				uint16_t unk48;
				uint16_t devblock_string_count;
				uint32_t cseg_size;
				uint16_t includes_count;
				uint16_t unk52;
				uint32_t unk54;
			};
			GscObjCW37* script = (GscObjCW37*)buff.get();


			// string table
			GSC_STRINGTABLE_ITEM* strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(script->magic + script->string_table);

			for (int j = 0; j < script->string_count; j++)
			{
				char* str = (char*)(script->magic + strings->string);

				handler->DecryptString(str);

				strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(reinterpret_cast<uint32_t*>(&strings[1]) + strings->num_address);
			}
			break;
		}
		case 0x38000a0d43534780: // CW 38
		{
			struct GscObjCW38 {
				uint8_t magic[8];
				int32_t crc;
				int32_t pad0c;
				uint64_t name;
				uint16_t string_count;
				uint16_t exports_count;
				uint16_t imports_count;
				uint16_t unk1e;
				uint16_t globalvar_count;
				uint16_t unk22;
				uint16_t includes_count;
				uint16_t devblock_string_count;
				uint32_t devblock_string_offset;
				uint32_t cseg_offset;
				uint32_t string_table;
				uint32_t includes_table;
				uint32_t exports_tables;
				uint32_t import_tables;
				uint32_t unk40;
				uint32_t globalvar_offset;
				uint32_t file_size;
				uint32_t unk4c;
				uint32_t cseg_size;
				uint32_t unk54;
			};
			GscObjCW38* script = (GscObjCW38*)buff.get();


			// string table
			GSC_STRINGTABLE_ITEM* strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(script->magic + script->string_table);

			for (int j = 0; j < script->string_count; j++)
			{
				char* str = (char*)(script->magic + strings->string);

				handler->DecryptString(str);

				strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(reinterpret_cast<uint32_t*>(&strings[1]) + strings->num_address);
			}
			break;
		}
		case 0xa0d4353478b: // MWIII 8B
		{
			struct GscObj23
			{
				uint8_t magic[8];
				uint64_t name;
				uint16_t unk10;
				uint16_t animtree_use_count;
				uint16_t animtree_count;
				uint16_t devblock_string_count;
				uint16_t export_count;
				uint16_t fixup_count;
				uint16_t unk1C;
				uint16_t imports_count;
				uint16_t includes_count;
				uint16_t unk22;
				uint16_t string_count;
				uint16_t unk26;
				uint32_t checksum;
				uint32_t animtree_use_offset;
				uint32_t animtree_offset;
				uint32_t cseg_offset;
				uint32_t cseg_size;
				uint32_t devblock_string_offset;
				uint32_t export_offset;
				uint32_t fixup_offset;
				uint32_t size1;
				uint32_t import_table;
				uint32_t include_table;
				uint32_t size2;
				uint32_t string_table;
			};

			GscObj23* script = (GscObj23*)buff.get();


			// string table
			GSC_STRINGTABLE_ITEM* strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(script->magic + script->string_table);

			for (int j = 0; j < script->string_count; j++)
			{
				char* str = (char*)(script->magic + strings->string);

				handler->DecryptString(str);

				strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(reinterpret_cast<uint32_t*>(&strings[1]) + strings->num_address);
			}

			// animtree tables

			GSC_ANIMTREE_ITEM* animt = reinterpret_cast<GSC_ANIMTREE_ITEM*>(script->magic + script->animtree_offset);

			for (int j = 0; j < script->animtree_count; j++)
			{
				handler->DecryptString((char*)(script->magic + animt->address_str1));
				handler->DecryptString((char*)(script->magic + animt->address_str2));

				animt = reinterpret_cast<GSC_ANIMTREE_ITEM*>(reinterpret_cast<uint32_t*>(&animt[1]) + animt->num_address);
			}

			GSC_USEANIMTREE_ITEM* animtu = reinterpret_cast<GSC_USEANIMTREE_ITEM*>(script->magic + script->animtree_use_offset);

			for (int j = 0; j < script->animtree_use_count; j++) {
				handler->DecryptString((char*)(script->magic + animtu->address));

				animtu = reinterpret_cast<GSC_USEANIMTREE_ITEM*>(reinterpret_cast<uint32_t*>(&animtu[1]) + animtu->num_address);
			}

			break;
		}
		case 0xa0d43534706: // Black Ops 6
		{
			struct GscObj24 {
				uint8_t magic[8];
				uint64_t name;
				uint16_t unk10;
				uint16_t animtree_use_count;
				uint16_t animtree_count;
				uint16_t devblock_string_count;
				uint16_t export_count;
				uint16_t fixup_count;
				uint16_t unk1C;
				uint16_t imports_count;
				uint16_t includes_count;
				uint16_t string_count;
				uint16_t unk24;
				uint16_t unk26;
				uint32_t checksum;
				uint32_t animtree_use_offset;
				uint32_t animtree_offset;
				uint32_t cseg_offset;
				uint32_t cseg_size;
				uint32_t devblock_string_offset;
				uint32_t export_offset;
				uint32_t fixup_offset;
				uint32_t size1;
				uint32_t import_table;
				uint32_t include_table;
				uint32_t string_table;
				uint32_t size2;
				uint32_t unk5C;
			};

			GscObj24* script = (GscObj24*)buff.get();

			// string table
			GSC_STRINGTABLE_ITEM* strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(script->magic + script->string_table);

			for (int j = 0; j < script->string_count; j++)
			{
				handler->DecryptString((char*)(script->magic + strings->string));
				strings = reinterpret_cast<GSC_STRINGTABLE_ITEM*>(reinterpret_cast<uint32_t*>(&strings[1]) + strings->num_address);
			}

		}
			break;
		default:
			ps::log::Print("WARNING", "Unknown magic %llx for file %s.", magic, fileName.c_str());
			return;
		}

		std::ofstream os{ std::format("{}/{}", gscsOutputPath, fileName), std::ios::binary };

		if (!os)
		{
			ps::log::Print("WARNING", "Error when writing %s 0x%llx", fileName.c_str(), fso->GetLastError());
			return;
		}

		os.write((const char*)buff.get(), size);

		os.close();
	});
}

PS_CINIT(ps::Command, ps::DecryptScriptCommand, ps::Command::GetCommands());