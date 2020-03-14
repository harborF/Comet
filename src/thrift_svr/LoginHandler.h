#ifndef _LOGIN_HANDLER_H__
#define _LOGIN_HANDLER_H__
#include "svr_job.h"

class LoginHandler : public BaseHandler
{
public:
    LoginHandler();
    ~LoginHandler();

public:
    int handleRequest(const int32_t msgId, const std::string& strInput, std::string& strOutput);
};

#endif // !_LOGIN_HANDLER_H__
