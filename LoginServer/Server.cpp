#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <process.h>

using namespace std;

//mysql
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

//socket
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32")

#define FD_SETSIZE				100

#include "MyUtility.h"
#include "Packet.h"
#include "PacketMaker.h"

fd_set Reads;
fd_set CopyReads;

const int HeaderSize = 4;

sql::Connection* Sql_Connection;

bool GetConfigFromFile(string& OutServer, string& OutUserName, string& OutPassword)
{
	ifstream ConfigFile("config.txt");
	if (!ConfigFile.is_open())
	{
		cout << "Error: Could not open config file." << endl;
		return false;
	}

	string Line;
	size_t Start;
	size_t End;
	while (getline(ConfigFile, Line)) {
		if (Start = Line.find("Server = ") != string::npos) {
			End = Line.find('\n', Start);
			OutServer = Line.substr(Start + 8, End);		// Extract server information
		}
		else if (Start = Line.find("Username = ") != string::npos)
		{
			End = Line.find('\n', Start);
			OutUserName = Line.substr(Start + 10, End);	// Extract username
		}
		else if (Start = Line.find("Password = ") != string::npos)
		{
			End = Line.find('\n', Start);
			OutPassword = Line.substr(Start + 10, End);	// Extract password
		}
	}
	ConfigFile.close();
	return true;
}

void DisconnectSocket(SOCKET& TargetSocket)
{
	closesocket(TargetSocket);
	FD_CLR(TargetSocket, &Reads);
	CopyReads = Reads;
}

bool RecvPacket(SOCKET& ClientSocket, pair<EPacket, char*>& OutPacketData)
{
	// Recv Header
	char HeaderBuffer[HeaderSize] = { 0, };
	int RecvByte = recv(ClientSocket, HeaderBuffer, HeaderSize, MSG_WAITALL);
	if (RecvByte == 0 || RecvByte < 0) //close, Error
	{
		cout << "Server Recv Error : " << GetLastError() << endl;
		return false;
	}

	unsigned short PayloadSize = 0;
	unsigned short PacketType = 0;

	memcpy(&PayloadSize, HeaderBuffer, 2);
	memcpy(&PacketType, &HeaderBuffer[2], 2);

	// network byte ordering
	PayloadSize = ntohs(PayloadSize);
	PacketType = ntohs(PacketType);

	printf("[Receive] Payload size : %d, Packet type : %d\n", PayloadSize, PacketType);
	OutPacketData.first = static_cast<EPacket>(PacketType);

	// Recv Payload
	if (PayloadSize > 0)
	{
		OutPacketData.second = new char[PayloadSize + 1];	// + 1 for '\0'

		RecvByte = recv(ClientSocket, OutPacketData.second, PayloadSize, MSG_WAITALL);
		if (RecvByte == 0 || RecvByte < 0)
		{
			cout << "Server Recv Error : " << GetLastError() << endl;
			return false;
		}
		OutPacketData.second[PayloadSize] = '\0';
		cout << "Payload : " << OutPacketData.second << endl;
	}

	return true;
}

void ProcessPacket(SOCKET& ClientSocket, const pair<EPacket, char*>& PacketData)
{
	bool bSendSuccess = false;
	sql::PreparedStatement* Sql_PreStatement = nullptr;
	sql::ResultSet* Sql_Result = nullptr;

	switch (PacketData.first)
	{
	case EPacket::C2S_ReqSignIn:
	{
		char* ColonPtr = strchr(PacketData.second, ':');
		if (ColonPtr != nullptr)
		{
			long long IDLen = ColonPtr - PacketData.second;

			string UserID(PacketData.second, IDLen);
			string UserPwd(ColonPtr + 1);

			cout << "ID : " << UserID << " Pwd : " << UserPwd << endl;

			// Check ID Exist in DB UserConfig
			string SqlQuery = "SELECT * FROM userconfig WHERE ID = ?";
			Sql_PreStatement = Sql_Connection->prepareStatement(SqlQuery);
			Sql_PreStatement->setString(1, UserID);
			Sql_Result = Sql_PreStatement->executeQuery();

			// If ID is valid
			if (Sql_Result->next()) {
				string dbPassword = Sql_Result->getString("Password");

				// If Password correct, Login Success!!
				if (UserPwd == dbPassword)
				{
					printf("Password Matched\n");

					string UserNickName = MyUtility::Utf8ToMultibyte(Sql_Result->getString("NickName"));

					bSendSuccess = PacketMaker::SendPacket(&ClientSocket, EPacket::S2C_ResSignIn_Success, UserNickName.c_str());
					if (!bSendSuccess)
					{
						cout << "Server Send Error : " << GetLastError() << endl;
						break;
					}
				}
				else
				{
					// If Password incorrect
					bSendSuccess = PacketMaker::SendPacket(&ClientSocket, EPacket::S2C_ResSignIn_Fail_InValidPassword);
					if (!bSendSuccess)
					{
						cout << "Server Send Error : " << GetLastError() << endl;
						break;
					}
				}
			}
			else
			{
				// else ID doesnt exist in db
				cout << "ID Does Not Exist." << endl;

				bSendSuccess = PacketMaker::SendPacket(&ClientSocket, EPacket::S2C_ResSignIn_Fail_InValidID);
				if (!bSendSuccess)
				{
					cout << "Server Send Error : " << GetLastError() << endl;
					break;
				}
			}
		}
	}
	break;
	case EPacket::C2S_ReqSignUpIDPwd:
	{
		char* ColonPtr = strchr(PacketData.second, ':');
		if (ColonPtr != nullptr)
		{
			long long IDLen = ColonPtr - PacketData.second;

			string NewUserID(PacketData.second, IDLen);
			string NewUserPwd(ColonPtr + 1);

			cout << "New ID : " << NewUserID << " New Pwd : " << NewUserPwd << endl;

			// Check ID Exist in DB UserConfig
			string SqlQuery = "SELECT * FROM userconfig WHERE ID = ?";
			Sql_PreStatement = Sql_Connection->prepareStatement(SqlQuery);
			Sql_PreStatement->setString(1, NewUserID);
			Sql_Result = Sql_PreStatement->executeQuery();

			if (Sql_Result->rowsCount() > 0)
			{
				// If ID is exist, error
				bSendSuccess = PacketMaker::SendPacket(&ClientSocket, EPacket::S2C_ResSignUpIDPwd_Fail_ExistID);
				if (!bSendSuccess)
				{
					cout << "Server Send Error : " << GetLastError() << endl;
					break;
				}
			}
			else
			{
				bSendSuccess = PacketMaker::SendPacket(&ClientSocket, EPacket::S2C_ResSignUpIDPwd_Success);
				if (!bSendSuccess)
				{
					cout << "Server Send Error : " << GetLastError() << endl;
					break;
				}
			}
		}
	}
	break;
	case EPacket::C2S_ReqSignUpNickName:
	{
		//string NewNickName(PacketData.second);

		//cout << "New Nick Name : " << NewNickName << endl;

		//// Check ID Exist in DB UserConfig
		//string SqlQuery = "SELECT * FROM userconfig WHERE NickName = ?";
		//Sql_PreStatement = Sql_Connection->prepareStatement(SqlQuery);
		//Sql_PreStatement->setString(1, MyUtility::MultibyteToUtf8(NewNickName));
		//Sql_Result = Sql_PreStatement->executeQuery();

		//if (Sql_Result->rowsCount() > 0)
		//{
		//	// If NickName is exist, error
		//	bSendSuccess = PacketMaker::SendPacket(&ClientSocket, EPacket::S2C_ResSignUpNickName_Fail_ExistNickName);
		//	if (!bSendSuccess)
		//	{
		//		cout << "Server Send Error : " << GetLastError() << endl;
		//		break;
		//	}
		//}
		//else
		//{
		//	// Create New Userconfig
		//	SqlQuery = "INSERT INTO userconfig(ID, Password, NickName) VALUES(?,?,?)";
		//	Sql_PreStatement = Sql_Connection->prepareStatement(SqlQuery);
		//	Sql_PreStatement->setString(1, TempUserList[ClientSocket].UserID);
		//	Sql_PreStatement->setString(2, TempUserList[ClientSocket].Password);
		//	Sql_PreStatement->setString(3, MyUtility::MultibyteToUtf8(NewNickName));
		//	Sql_PreStatement->execute();

		//	TempUserList.erase(ClientSocket);

		//	bSendSuccess = PacketMaker::SendPacket(&ClientSocket, EPacket::S2C_ResSignUpNickName_Success);
		//	if (!bSendSuccess)
		//	{
		//		cout << "Server Send Error : " << GetLastError() << endl;
		//		break;
		//	}
		//}
	}
	break;
	default:
		break;
	}

	delete Sql_Result;
	delete Sql_PreStatement;
}

int main()
{
	cout << "Connecting to DB Server... ";

	string Server;
	string Username;
	string Password;
	if (!GetConfigFromFile(Server, Username, Password))
	{
		cout << "Fail." << endl;
		cout << "Get Config txt Error" << endl;
		system("pause");
		exit(-1);
	}

	sql::Driver* Sql_Driver;
	try
	{
		Sql_Driver = get_driver_instance();
		Sql_Connection = Sql_Driver->connect(Server, Username, Password);
		cout << "Done!" << endl;
	}
	catch (sql::SQLException e)
	{
		cout << "Fail." << endl;
		cout << "Could not connect to data base : " << e.what() << endl;
		system("pause");
		exit(-1);
	}

	Sql_Connection->setSchema("skiski");

	cout << "Starting Login Server... ";

	WSADATA WsaData;
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
	{
		cout << "Fail." << endl;
		cout << "Error On StartUp : " << GetLastError() << endl;
		system("pause");
		exit(-1);
	}

	SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "Fail." << endl;
		cout << "ListenSocket Error : " << GetLastError() << endl;
		system("pause");
		exit(-1);
	}

	SOCKADDR_IN ListenSockAddr;
	memset(&ListenSockAddr, 0, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	ListenSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	ListenSockAddr.sin_port = htons(8881);

	if (_WINSOCK2API_::bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr)) == SOCKET_ERROR)
	{
		cout << "Fail." << endl;
		cout << "Bind Error : " << GetLastError() << endl;
		system("pause");
		exit(-1);
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cout << "Fail." << endl;
		cout << "Listen Error : " << GetLastError() << endl;
		system("pause");
		exit(-1);
	}

	struct timeval Timeout;
	Timeout.tv_sec = 0;
	Timeout.tv_usec = 500;

	FD_ZERO(&Reads);
	FD_SET(ListenSocket, &Reads);

	cout << "Done!" << endl;
	cout << "Wait for Connecting... " << endl;

	while (true)
	{
		CopyReads = Reads;

		int ChangeSocketCount = select(0, &CopyReads, 0, 0, &Timeout);
		if (ChangeSocketCount > 0)
		{
			for (int i = 0; i < (int)Reads.fd_count; ++i)
			{
				if (FD_ISSET(Reads.fd_array[i], &CopyReads))
				{
					//connect
					if (Reads.fd_array[i] == ListenSocket)
					{
						SOCKADDR_IN ClientSocketAddr;
						memset(&ClientSocketAddr, 0, sizeof(ClientSocketAddr));
						int ClientSockAddrLength = sizeof(ClientSocketAddr);

						SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSocketAddr, &ClientSockAddrLength);
						if (ClientSocket == INVALID_SOCKET)
						{
							cout << "Accept Error : " << GetLastError() << endl;
							continue;
						}

						FD_SET(ClientSocket, &Reads);
						CopyReads = Reads;
						char IP[1024] = { 0, };
						inet_ntop(AF_INET, &ClientSocketAddr.sin_addr.s_addr, IP, 1024);
						printf("[%d] Connected : %s\n", (unsigned short)ClientSocket, IP);

						// Recv packet from client and process it
						pair<EPacket, char*> RecvPacketData;
						if (RecvPacket(ClientSocket, RecvPacketData))
						{
							ProcessPacket(ClientSocket, RecvPacketData);
						}

						//disconnect Socket
						DisconnectSocket(ClientSocket);

						break;
					}
				}
			}
		}
		else
		{
			// when no changes on socket count while timeout
		}
	}

	// Clean Up
	closesocket(ListenSocket);
	WSACleanup();

	delete Sql_Connection;

	system("pause");

	return 0;
}
