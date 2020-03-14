namespace cpp protruly
#include "User.thrift"

struct ST_SvrInfo{
	1: required string strSvrHost,
	2: required i32 uiThreadIdx,
}

struct ST_UserInfo{
	1: required i32 member_id,
	2: required i32 channel,
	3: required i32 type,
	4: required string session,
}

struct LoginResult{
	 1: required i32 retCode = -1,
	 2: required ST_UserInfo info,
}

/*ÏûÏ¢·µ»Ø*/
struct ST_MsgResult{
	 1: required i32 retCode = -1,
	 2: optional string jsData,
}

service StateSvr {
	LoginResult login(1: ST_SvrInfo info, 2: string strParam),
	oneway void logout(1: ST_UserInfo info),
	
	ST_MsgResult handleRequest(1: i32 msgId, 2: string jsParam);
}
