// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LoginSubsystem.generated.h"

/**
 * web server logic
 */

enum class ELoginPacket
{
	None = 0,

	S2C_Ping = 1,
	C2S_Ping = 2,

	S2C_CastMessage = 3,
	//C2S_CastMessage										= 4,	//reserved

	S2C_ConnectSuccess = 100,

	C2S_ReqSignIn = 1000,
	S2C_ResSignIn_Success = 1001,
	S2C_ResSignIn_Fail_InValidID = 1002,
	S2C_ResSignIn_Fail_InValidPassword = 1003,
	//S2C_ResSignIn_Fail_AlreadySignIn						= 1004,  //reserved

	C2S_ReqSignUpIDPwd = 1010,
	S2C_ResSignUpIDPwd_Success = 1011,
	S2C_ResSignUpIDPwd_Fail_ExistID = 1012,

	C2S_ReqSignUpNickName = 1020,
	S2C_ResSignUpNickName_Success = 1021,
	S2C_ResSignUpNickName_Fail_ExistNickName = 1022,

	C2S_ReqMatchMaking = 1100,
	S2C_ResMatchMaking_DediIP = 1101,

	C2S_ReqCancelMatchMaking = 1110,

	Max,
};

struct FLoginPacketData
{
public:
	FLoginPacketData() : PacketType(ELoginPacket::None), Payload() {}
	FLoginPacketData(ELoginPacket NewPacketType) : PacketType(NewPacketType), Payload() {}
	FLoginPacketData(ELoginPacket NewPacketType, FString Payload) : PacketType(NewPacketType), Payload(Payload) {}
	FLoginPacketData(uint16_t NewPacketTypeInt, FString Payload) : PacketType(static_cast<ELoginPacket>(NewPacketTypeInt)), Payload(Payload) {}

	ELoginPacket PacketType;
	FString Payload;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FDele_RecvPacket, const FString&, const int32&, bool);

const int32 HeaderSize{ 4 };
UCLASS()
class SKISKI_API ULoginSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	ULoginSubsystem();

protected:
	FSocket* Connect(const int32& PortNum, const FString& IP);

// Do not use this Funcs
public:
	static bool Recv(TSharedPtr<FSocket> LoginSocket, FLoginPacketData& OutRecvPacket);

	void DestroySocket(FSocket*& TargetSocket);

	FORCEINLINE void SetRecvPacket(const FLoginPacketData& RecvPacket) { RecvPacketData = RecvPacket; };

// Funcs You Can Use in other classes
public:
	bool Send(const FLoginPacketData& SendPacket);

// Your Login Server configs
protected:
	const int32 ServerPort = 8881;
	const FString ServerIP = TEXT("127.0.0.1");

// MultiCast Delegates You Can Use
public:
	FDele_RecvPacket RecvPacketDelegate;

protected:
	// For Socket Error Log
	void PrintSocketError();

protected:
// You Can Write Your Custom Codes In this Func
	UFUNCTION()
	void ManageRecvPacket();

/* Your Custom Vars In Here----------------------------*/
public:
	FORCEINLINE const FString& GetIDPwd() const { return IDPwd; };
	FORCEINLINE void SetIDPwd(const FString& NewIDPwd) { IDPwd = NewIDPwd; };

	FORCEINLINE const FString& GetUserNickName() const { return UserNickName; };
	FORCEINLINE void SetUserNickName(const FString& NewNickName) { UserNickName = NewNickName; };

protected:
	FString IDPwd;

	FString UserNickName;
/*-----------------------------------------------------*/

protected:
	//class FRecvThread* RecvThread;

	FLoginPacketData RecvPacketData;

	FTimerHandle ManageRecvPacketHandle;
};

class SKISKI_API FRecvThread : public FRunnable
{
public:
	FRecvThread(TSharedPtr<FSocket> LoginSocket);
	virtual ~FRecvThread();

protected:
	virtual uint32 Run() override;

	virtual void Exit() override;

private:
	FRunnableThread* Thread;

private:
	TSharedPtr<FSocket> Socket;

};
