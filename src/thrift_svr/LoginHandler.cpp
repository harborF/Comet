#include "LoginHandler.h"

LoginHandler::LoginHandler()
{
}

LoginHandler::~LoginHandler()
{
}

int LoginHandler::handleRequest(const int32_t msgId, const std::string& strInput, std::string& strOutput)
{
    Json::FastWriter writer;
    Json::Value jsInput, jsOutput;
    Json::Reader reader(Json::Features::strictMode());
    if (!reader.parse(strInput, jsInput)) {
        acl_msg_error("[%s]>>parse json", __func__);
        return -1;
    }

    static uint32_t i = 0;

    jsOutput["code"] = 0;
    jsOutput["count"] = i++;
    strOutput = writer.write(jsOutput);

    return 0;
}
