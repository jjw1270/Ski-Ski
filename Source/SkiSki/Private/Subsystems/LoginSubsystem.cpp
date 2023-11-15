// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginSubsystem.h"

/* Custom Includes Here--------------------------------*/
#include "SkiSki.h"
/*-----------------------------------------------------*/

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "IPAddress.h"
#include "terse/utils/Endianness.h"

ULoginSubsystem::ULoginSubsystem()
{
}

FSocket* ULoginSubsystem::Connect(const int32& PortNum, const FString& IP)
{
	// Create
	FSocket* LoginSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("LoginSocket"), false);
	if (!LoginSocket)
	{
		ABLOG(Error, TEXT("Create Socket Failure"));
		return nullptr;
	}

	// Bind
	FIPv4Address IPv4Address;
	FIPv4Address::Parse(IP, IPv4Address);

	TSharedPtr<FInternetAddr> SocketAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	SocketAddress->SetPort(PortNum);
	SocketAddress->SetIp(IPv4Address.Value);

	// Connect
	if (!LoginSocket->Connect(*SocketAddress))
	{
		ABLOG(Error, TEXT("Connect Server Fail"));
		PrintSocketError();

		DestroySocket(LoginSocket);

		return nullptr;
	}

	ABLOG(Warning, TEXT("Connect Server Success!"));
	return LoginSocket;
}

void ULoginSubsystem::DestroySocket(FSocket*& TargetSocket)
{
	if (TargetSocket)
	{
		if (TargetSocket->GetConnectionState() == SCS_Connected)
		{
			TargetSocket->Close();
			ABLOG(Warning, TEXT("Close Socket"));
		}

		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(TargetSocket);
		ABLOG(Warning, TEXT("Destroy Socket"));
	}
}

void ULoginSubsystem::PrintSocketError()
{
	ESocketErrors SocketErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
	const TCHAR* SocketError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError(SocketErrorCode);

	UE_LOG(LogSockets, Error, TEXT("Socket Error : %s (%d)"), SocketError, (int32)SocketErrorCode);
}

void ULoginSubsystem::ManageRecvPacket()
{
	// Process packet
	if (RecvPacketDelegate.IsBound())
	{
		int32 PacketCode = static_cast<int32>(RecvPacketData.PacketType);

		switch (RecvPacketData.PacketType)
		{
		case ELoginPacket::S2C_ConnectSuccess:
			RecvPacketDelegate.Broadcast(TEXT("로그인 서버 접속 성공"), PacketCode, true);
			break;
		case ELoginPacket::S2C_ResSignIn_Fail_InValidID:
			SetIDPwd("");
			RecvPacketDelegate.Broadcast(TEXT("등록되지 않은 아이디 입니다"), PacketCode, false);
			break;
		case ELoginPacket::S2C_ResSignIn_Fail_InValidPassword:
			RecvPacketDelegate.Broadcast(TEXT("비밀번호가 일치하지 않습니다"), PacketCode, false);
			break;
		case ELoginPacket::S2C_ResSignIn_Success:
			SetUserNickName(RecvPacketData.Payload);
			RecvPacketDelegate.Broadcast(FString::Printf(TEXT("환영합니다 %s 님!"), *RecvPacketData.Payload), PacketCode, true);
			break;
		case ELoginPacket::S2C_ResSignUpIDPwd_Success:
			RecvPacketDelegate.Broadcast(TEXT("새로운 닉네임을 입력하세요"), PacketCode, true);
			break;
		case ELoginPacket::S2C_ResSignUpIDPwd_Fail_ExistID:
			RecvPacketDelegate.Broadcast(TEXT("아이디가 이미 존재합니다"), PacketCode, false);
			break;
		case ELoginPacket::S2C_ResSignUpNickName_Success:
			RecvPacketDelegate.Broadcast(TEXT("등록되었습니다!"), PacketCode, true);
			break;
		case ELoginPacket::S2C_ResSignUpNickName_Fail_ExistNickName:
			RecvPacketDelegate.Broadcast(TEXT("닉네임이 이미 존재합니다"), PacketCode, false);
			break;
		case ELoginPacket::S2C_ResMatchMaking_DediIP:
			RecvPacketDelegate.Broadcast(RecvPacketData.Payload, PacketCode, false);
		default:
			break;
		}
	}

	// clear
	RecvPacketData = FLoginPacketData();
	GetWorld()->GetTimerManager().ClearTimer(ManageRecvPacketHandle);
}

bool ULoginSubsystem::Recv(TSharedPtr<FSocket> LoginSocket, FLoginPacketData& OutRecvPacket)
{
	if (!LoginSocket)
	{
		ABLOG(Error, TEXT("Socket is null"));
		return false;
	}

	if (LoginSocket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(.5f)))
	{
		TArray<uint8_t> HeaderBuffer;
		HeaderBuffer.AddZeroed(HeaderSize);

		// Recv Header
		int32 BytesRead = 0;
		bool bRecvHeader = LoginSocket->Recv(HeaderBuffer.GetData(), HeaderSize, BytesRead, ESocketReceiveFlags::WaitAll);
		if (!bRecvHeader)
		{
			//PrintSocketError(TEXT("Receive Header"));
			return false;
		}

		uint16 RecvPayloadSize;
		uint16 RecvPacketType;

		// Get Size and Type from HeaderBuffer
		FMemory::Memcpy(&RecvPayloadSize, HeaderBuffer.GetData(), sizeof(uint16_t));
		FMemory::Memcpy(&RecvPacketType, HeaderBuffer.GetData() + sizeof(uint16_t), sizeof(uint16_t));

		/* I Skip Network Byte Ordering because most of game devices use little endian */
		RecvPayloadSize = ntoh(RecvPayloadSize);
		RecvPacketType = ntoh(RecvPacketType);

		OutRecvPacket.PacketType = static_cast<ELoginPacket>(RecvPacketType);

		// Recv Payload
		if (RecvPayloadSize > 0)
		{
			uint8_t* PayloadBuffer = new uint8_t[RecvPayloadSize + 1];

			BytesRead = 0;
			bool bRecvPayload = LoginSocket->Recv(PayloadBuffer, RecvPayloadSize, BytesRead, ESocketReceiveFlags::WaitAll);

			if (!bRecvPayload)
			{
				//PrintSocketError(TEXT("Receive Payload"));
				return false;
			}
			PayloadBuffer[RecvPayloadSize] = '\0';

			//Utf8 to FStirng
			FString PayloadString;
			PayloadString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(PayloadBuffer)));

			OutRecvPacket.Payload = PayloadString;

			delete[] PayloadBuffer;
			PayloadBuffer = nullptr;
		}

		ABLOG(Warning, TEXT(" [Recv] PacketType : %d, PayloadSize : %d"), RecvPacketType, RecvPayloadSize);
	}
	return true;
}

bool ULoginSubsystem::Send(const FLoginPacketData& SendPacket)
{
	// Connect to Server And Send Packet, Start Recv Thread

	TSharedPtr<FSocket> LoginSocket = MakeShareable<FSocket>(Connect(ServerPort, ServerIP));
	if (!LoginSocket)
	{
		return false;
	}

	ABLOG(Warning, TEXT("LoginSocket count : %d"), LoginSocket.GetSharedReferenceCount());

	uint8_t* PayloadBuffer = nullptr;
	uint16_t PayloadSize = 0;

	// Set Payload Size
	if (!SendPacket.Payload.IsEmpty())
	{
		// FString to UTF8 const char* type buffer
		ANSICHAR* PayloadCharBuf = TCHAR_TO_UTF8(*SendPacket.Payload);
		PayloadSize = strlen(PayloadCharBuf);
		PayloadBuffer = reinterpret_cast<uint8_t*>(PayloadCharBuf);
	}

	// Send Header
	ABLOG(Warning, TEXT("Payload Size : %d"), (int32)PayloadSize);

	const uint16_t Type = static_cast<uint16_t>(SendPacket.PacketType);

	uint8_t HeaderBuffer[HeaderSize] = { 0, };

	FMemory::Memcpy(&HeaderBuffer, &PayloadSize, 2);
	FMemory::Memcpy(&HeaderBuffer[2], &Type, 2);

	int32 BytesSent = 0;
	if (!LoginSocket->Send(HeaderBuffer, HeaderSize, BytesSent))
	{
		ABLOG(Error, TEXT("Send Error"));
		PrintSocketError();
		return false;
	}

	if (PayloadBuffer != nullptr)
	{
		BytesSent = 0;
		if (!LoginSocket->Send(PayloadBuffer, PayloadSize, BytesSent))
		{
			ABLOG(Error, TEXT("Send Error"));
			PrintSocketError();
			return false;
		}
	}

	// Start Recv Thread
	FRecvThread* RecvThread = new FRecvThread(LoginSocket);

	//GetWorld()->GetTimerManager().SetTimer(ManageRecvPacketHandle, this, &ULoginSubsystem::ManageRecvPacket, 0.1f, true);

	return true;
}

FRecvThread::FRecvThread(TSharedPtr<FSocket> LoginSocket) : Socket(LoginSocket)
{
	Thread = FRunnableThread::Create(this, TEXT("RecvThread"));
}

FRecvThread::~FRecvThread()
{
	if (Thread)
	{
		Thread->Kill(true);
		delete Thread;
	}
}

uint32 FRecvThread::Run()
{
	while (true)
	{
		FLoginPacketData PacketData;
		bool RecvByte = ULoginSubsystem::Recv(Socket, PacketData);
		if (!RecvByte)
		{
			ABLOG(Error, TEXT("Recv Error, Stop Thread"));
			break;
		}

		if (PacketData.PacketType != ELoginPacket::None)
		{
			//LoginSubsystem->SetRecvPacket(PacketData);
			ABLOG(Warning, TEXT("Recv Success!"));
			break;
		}
	}

	return 0;
}

void FRecvThread::Exit()
{
	ABLOG(Warning, TEXT("FRecvThread::Exit"));
}
