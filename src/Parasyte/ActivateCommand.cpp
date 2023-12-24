#include "pch.h"
// #include "ActivateCommand.h"
// #include "ProcessDumper.h"
// #include <iostream>
// #include <string>
// #include <windows.h>
// #include <iphlpapi.h>
// #include <intrin.h>
//
// #pragma comment(lib, "iphlpapi.lib")
//
// ps::ActivateCommand::ActivateCommand() : Command(
// 	"activate",
// 	"Creates a Cordycep-Compatible dump of the provided exe.",
// 	"This command handles automatically creating a Cordycep-Compatible dump of the exe you provide it.",
// 	"dump \"C:\\iw6mp64_ship.exe\"",
// 	false)
// {
//
// }
//
// void ps::ActivateCommand::Execute(ps::CommandParser& parser) const
// {
// 	unsigned char MACData[6];
// 	ULONG MACDataLen = sizeof(MACData);
//
// 	// Fetch MAC address (of the first network adapter in most cases)
// 	if (GetAdaptersInfo(NULL, &MACDataLen) == ERROR_BUFFER_OVERFLOW) {
// 		IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO*)malloc(MACDataLen);
// 		if (GetAdaptersInfo(pAdapterInfo, &MACDataLen) == NO_ERROR) {
// 			memcpy(MACData, pAdapterInfo->Address, 6);
// 			free(pAdapterInfo);
// 		}
// 		else {
// 			free(pAdapterInfo);
// 		}
// 	}
// }
//
// PS_CINIT(ps::Command, ps::ActivateCommand, ps::Command::GetCommands());