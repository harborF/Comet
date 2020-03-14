#include "svr_job.h"
#include "LoginHandler.h"

BaseHandler::BaseHandler()
{
}

BaseHandler::~BaseHandler()
{
}

////////////////////////////////////////////////////////////
struct ST_Handler {
    uint32_t uiCmdId;
    BaseHandler* handler;
}opr_handler[] = {
    {100, new LoginHandler()}
};

StateSvrHandler::StateSvrHandler() {
    cluster_.set("127.0.0.1:6379", 10, 2, 2);
    for (uint32_t i = 0; i < sizeof(opr_handler)/sizeof(opr_handler[0]);++i) {
        map_handler_[opr_handler[i].uiCmdId] = opr_handler[i].handler;
    }
}

void StateSvrHandler::login(LoginResult& _return, const ST_SvrInfo& info, const std::string& strParam) {
    // Your implementation goes here
    acl_msg_info("[%s]%s", __func__, strParam.c_str());

    acl::redis cmd(&cluster_);
    
    static uint32_t i = 1000;
    _return.retCode = 0;
    _return.info.member_id = i++;
    _return.info.channel = 1;
    _return.info.type = 1;
    char szBuf[256];
    snprintf(szBuf, 256, "s_%d", i);
    _return.info.session = szBuf;
}

void StateSvrHandler::logout(const ST_UserInfo& info) {
    // Your implementation goes here
    acl_msg_info("[%s]", __func__);
}

void StateSvrHandler::handleRequest(ST_MsgResult& _return, const int32_t uiMsgId, const std::string& strInput)
{
    acl_msg_info("[%s]command-%d", __func__, uiMsgId);
    Hash_Handler::const_iterator itF = map_handler_.find(uiMsgId);
    if (itF != map_handler_.end()) {
        if (0 == (_return.retCode = itF->second->handleRequest(uiMsgId, strInput, _return.jsData))) {
            _return.__isset.jsData = true;
        }
    }
    else {
        acl_msg_error("handler not found command(%u)", uiMsgId);
    }
}
