#ifndef _SVR_JOB_H__
#define _SVR_JOB_H__
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "StateSvr.h"
#include "json/json.h"
#include <unordered_map>

using namespace  ::protruly;

class BaseHandler
{
public:
    BaseHandler();
    virtual ~BaseHandler();
public:
    virtual int handleRequest(const int32_t msgId, const std::string& strInput, std::string& strOutput) = 0;
};

class StateSvrHandler : virtual public StateSvrIf
{
    typedef std::unordered_map<uint32_t, BaseHandler*> Hash_Handler;
public:
    StateSvrHandler();

    void login(LoginResult& _return, const ST_SvrInfo& info, const std::string& strParam);
    void logout(const ST_UserInfo& info);

    void handleRequest(ST_MsgResult& _return, const int32_t uiMsgId, const std::string& strInput);
private:
    Hash_Handler map_handler_;
    acl::redis_client_cluster cluster_;
};

#endif // !_SVR_JOB_H__
