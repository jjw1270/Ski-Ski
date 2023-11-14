// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EngineMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(MyLog, Log, All);

#define ABLOG_CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))

// 코드가 들어있는 파일 이름과 함수, 그리고 라인 정보를 추가해서 MyLog 카테고리로 로그를 남긴다.
// 로그를 사용한 함수의 실행 시점을 파악할 때 유용하다.
#define ABLOG_S(Verbosity) UE_LOG(MyLog, Verbosity, TEXT("%s"), *ABLOG_CALLINFO)

// 정보에 형식 문자열로 추가 정보를 지정해 로그를 남긴다.
#define ABLOG(Verbosity, Format, ...) UE_LOG(MyLog, Verbosity, TEXT("%s%s"), *ABLOG_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))

#define CHECK_VALID(Obj) \
if (!(Obj)) \
{ \
    ABLOG(Error, TEXT("NULL = %s : %s"), *GetClass()->GetName(), TEXT(#Obj)); \
    return; \
}
