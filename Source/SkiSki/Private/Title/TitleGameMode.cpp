// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TitleGameMode.h"
#include "SkiSki.h"
#include "LoginSubsystem.h"

void ATitleGameMode::StartPlay()
{
	Super::StartPlay();

	UGameInstance* GI = GetGameInstance();
	CHECK_VALID(GI);

	LoginSubSystem = GI->GetSubsystem<ULoginSubsystem>();
	CHECK_VALID(LoginSubSystem);

	ABLOG(Warning, TEXT("Send Ping"));
	FLoginPacketData Packet(ELoginPacket::C2S_Ping);
	LoginSubSystem->Send(Packet);
}
