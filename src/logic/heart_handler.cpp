#include "opr_handler.h"

int heart_handler(const NetPkgHeader& stHdr, MasterTcpFiber* fiber,
    const void* pData, const unsigned int uiLen)
{
    acl_msg_info("[%s]fiber-%ld-%d: member-%u", 
        __func__, fiber->getThread()->thread_id(), fiber->get_id(),
        fiber->getMemberId());
    
#ifdef USE_THRIFT
    std::string strAuthParam("{\"cmd\":\"hello\"}");
    ThriftProxy proxy(stHdr.uiCmdId, fiber->getThread()->getSockPool());
    try {
        StateSvrClient client_1(proxy.getMultiProtocl("StateSvr"));

        ST_MsgResult msgRet;
        client_1.handleRequest(msgRet, 100, strAuthParam);
        acl_msg_info("[%s]>>thrift command:100 ret code:%d message:%s",
            __func__, msgRet.retCode, msgRet.jsData.c_str());

    }
    catch (TException& e) {
        proxy.clear();
        acl_msg_error("[%s]>>thrift exception: %s\n", __func__, e.what());
    }
#endif

    return fiber->sendMsg(ConstUtils::kCmdHeartAck, NULL, 0);
}

int heart_handler(const NetPkgHeader&, MasterWSFiber* ws,
    const void* pData, const unsigned int uiLen)
{
    acl_msg_info("[%s]fiber-%ld-%d: member-%u", 
        __func__, ws->getThread()->thread_id(), ws->get_id(),
        ws->getMemberId());

    return ws->sendMsg(ConstUtils::kCmdHeartAck, NULL, 0);
}
