#include "opr_handler.h"

struct ST_Handler {
    unsigned int uiCmdId;
    OprHandler handler;
}opr_handler[] = {
    {ConstUtils::kCmdHeartReq, heart_handler},
    {ConstUtils::kCmdAuthReq, login_handler},

    {ConstUtils::kCmdPushMsgAck, push_ack_handler},
    {0, NULL }
};

#ifdef USE_THRIFT
int default_handler(const NetPkgHeader& stHdr, MasterTcpFiber* fiber,
    const void* pData, const unsigned int uiLen)
{
    try {
        const std::string strParam((char*)pData, uiLen);

        MasterThread* pThread = fiber->getThread();
        ThriftProxy proxy(stHdr.uiCmdId, pThread->getSockPool());
        StateSvrClient client(proxy.getProtocl());

        ST_MsgResult msgRet;
        client.handleRequest(msgRet, stHdr.uiCmdId, strParam);
        acl_msg_info("[%s]>>thrift command:%u >> code:%d message:%s",
            __func__, stHdr.uiCmdId, msgRet.retCode, msgRet.jsData.c_str());
        if (!msgRet.jsData.empty()) {
            return fiber->sendMsg(stHdr.uiCmdId + 1, msgRet.jsData);
        }
        return fiber->sendMsg(stHdr.uiCmdId + 1, msgRet.retCode);
    }
    catch (TException& e) {
        acl_msg_error("[%s]>>thrift exception: %s\n", __func__, e.what());
    }

    return fiber->sendMsg(stHdr.uiCmdId + 1, ConstUtils::kErrInner);
}
#endif

OprHandler getHandler(const unsigned int uiCmdId)
{
    for (unsigned int i = 0; i < sizeof(opr_handler) / sizeof(opr_handler[0]); ++i) {
        if (opr_handler[i].uiCmdId == uiCmdId) {
            return opr_handler[i].handler;
        }
    }
#ifdef USE_THRIFT
    return default_handler;
#else
    return NULL;
#endif
}

///////////////////////////////////////////////////////
struct ST_HttpHandler {
    const char* szUrl;
    HttpHandler handler;
}http_handler[] = {
    {"/PushMsg", push_handler},
    {"/NotifyLogout", logout_handler }
};

HttpHandler getHttpHandler(const char* szUrl)
{
    for (unsigned int i = 0; i < sizeof(http_handler) / sizeof(http_handler[0]); ++i) {
        if (0 == strcmp(http_handler[i].szUrl, szUrl)) {
            return http_handler[i].handler;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////
struct ST_WsHandler {
    unsigned int uiCmdId;
    WSHandler handler;
}ws_handler[] = {
    {ConstUtils::kCmdHeartReq, heart_handler},
    {ConstUtils::kCmdAuthReq, login_handler},

    {ConstUtils::kCmdPushMsgAck, push_ack_handler},
    {0, NULL }
};

WSHandler getWSHandler(const unsigned int uiCmdId)
{
    for (unsigned int i = 0; i < sizeof(ws_handler) / sizeof(ws_handler[0]); ++i) {
        if (ws_handler[i].uiCmdId == uiCmdId) {
            return ws_handler[i].handler;
        }
    }

    return NULL;
}

