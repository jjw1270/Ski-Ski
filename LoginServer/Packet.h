#pragma once

#ifndef __PACKET_H__ 
#define __PACKET_H__

enum class EPacket
{
	None													= 0,

	S2C_Ping												= 1,
	C2S_Ping												= 2,

	S2C_CastMessage											= 3,
	//C2S_CastMessage										= 4,	//reserved

	S2C_ConnectSuccess										= 100,

	///////////////////////////////////////////////////////////////////////////

	C2S_ReqSignIn											= 1000,
	S2C_ResSignIn_Success									= 1001,
	S2C_ResSignIn_Fail_InValidID							= 1002,
	S2C_ResSignIn_Fail_InValidPassword						= 1003,
	//S2C_ResSignIn_Fail_AlreadySignIn						= 1004,  //reserved

	C2S_ReqSignUpIDPwd										= 1010,
	S2C_ResSignUpIDPwd_Success								= 1011,
	S2C_ResSignUpIDPwd_Fail_ExistID							= 1012,

	C2S_ReqSignUpNickName									= 1020,
	S2C_ResSignUpNickName_Success							= 1021,
	S2C_ResSignUpNickName_Fail_ExistNickName				= 1022,
	
	C2S_ReqMatchMaking										= 1100,
	S2C_ResMatchMaking_DediIP								= 1101,

	C2S_ReqCancelMatchMaking								= 1110,

	///////////////////////////////////////////////////////////////////////////

	C2S_ReqDediTCPConnect									= 3000,

	S2C_ReqDediServer										= 3100,
	C2S_ResDediServer										= 3101,

	///////////////////////////////////////////////////////////////////////////

	Max,
};

#pragma pack(1)
#pragma pack()

#endif