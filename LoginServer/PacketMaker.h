#pragma once

#include "Packet.h"
#include <utility>
#include <string>
#include <map>
using namespace std;

#include <WinSock2.h>

class PacketMaker
{
protected:
	static const int HeaderSize = 4;

public:
	static bool SendPacket(SOCKET* ClientSocket, EPacket PacketToSend);

	static bool SendPacket(SOCKET* ClientSocket, EPacket PacketToSend, const char* MessageToSend);

	//// broadcasting. doesnt need ACK
	//static void SendPacketToAllConnectedClients(const map<unsigned short, UserData>& UserList, EPacket PacketToSend, const char* MessageToSend);

	//// broadcasting. doesnt need ACK. Param : Excep User NickName
	//static void SendPacketToAllConnectedClients(const map<unsigned short, UserData>& UserList, EPacket PacketToSend, const char* MessageToSend, unsigned short ExcepUserNumber);

protected:
	// Use this PacketMaker if does not have params
	static pair<char*, int> MakeBuffer(EPacket Type);

	// const char* params
	static pair<char*, int> MakeBuffer(EPacket Type, const char* NewData);

protected:
	static char* MakeHeader(char* Buffer, EPacket Type, unsigned short Size);
};
