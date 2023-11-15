#include "PacketMaker.h"
#include <iostream>
#include <string>
using namespace std;

bool PacketMaker::SendPacket(SOCKET* ClientSocket, EPacket PacketToSend)
{
	cout << "SendPacket" << endl;

	pair<char*, int> BufferData = MakeBuffer(PacketToSend);

	int SendByte = send(*ClientSocket, BufferData.first, BufferData.second, 0);
	if (SendByte <= 0)
	{
		cout << "Send Error : " << GetLastError() << endl;
		return false;
	}
	return true;
}

bool PacketMaker::SendPacket(SOCKET* ClientSocket, EPacket PacketToSend, const char* MessageToSend)
{
	cout << "SendPacket" << endl;

	pair<char*, int> BufferData = MakeBuffer(PacketToSend, MessageToSend);

	int SendByte = send(*ClientSocket, BufferData.first, BufferData.second, 0);
	if (SendByte <= 0)
	{
		cout << "Send Error" << GetLastError() << endl;
		return false;
	}
	return true;
}

pair<char*, int> PacketMaker::MakeBuffer(EPacket Type)
{
	char* Buffer = new char[HeaderSize];

	MakeHeader(Buffer, Type, 0);

	return make_pair(Buffer, HeaderSize);
}

pair<char*, int> PacketMaker::MakeBuffer(EPacket Type, const char* NewData)
{
	// Header       Data
	//[][][][] [Variable data]

	const int PayloadSize = (int)strlen(NewData);
	const int BufferSize = HeaderSize + PayloadSize;

	char* Buffer = new char[BufferSize];
	MakeHeader(Buffer, Type, PayloadSize);

	memcpy(&Buffer[HeaderSize], NewData, PayloadSize);

	return make_pair(Buffer, BufferSize);
}

char* PacketMaker::MakeHeader(char* Buffer, EPacket Type, unsigned short PayloadSize)
{
	// Header
	//size code
	//[][] [][]

	unsigned short size = htons(PayloadSize);
	unsigned short code = htons(static_cast<unsigned short>(Type));

	memcpy(Buffer, &PayloadSize, 2);
	memcpy(&Buffer[2], &code, 2);

	return Buffer;
}
