#include "opr_handler.h"

int login_handler(const NetPkgHeader&, MasterTcpFiber* fiber,
    const void* pData, const unsigned int uiLen)
{
    const std::string strReq((char*)pData, uiLen);

    Json::Value jsInput;
    Json::Reader reader(Json::Features::strictMode());
    if (!reader.parse(strReq, jsInput)) {
        acl_msg_error("[%s]fiber-%ld-%d: %s", __func__, fiber->getThread()->thread_id(), fiber->get_id(), strReq.c_str());
        return fiber->sendMsg(ConstUtils::kCmdAuthAck, ConstUtils::kErrBodyParse);
    }

    if (jsInput["member_id"].isInt64()) {
        fiber->setLoginInfo(jsInput["member_id"].asInt64());
    }

    Json::Value jsOutput;
    if (fiber->getThread()->getConnMgr()->login(strReq, fiber, jsOutput)) {
        acl_msg_error("[%s]fiber-%ld-%d: %s", __func__, fiber->getThread()->thread_id(), fiber->get_id(), strReq.c_str());
        return fiber->sendMsg(ConstUtils::kCmdAuthAck, ConstUtils::kErrSign);
    }
    jsOutput["code"] = ConstUtils::kSeccuss;
    acl_msg_info("[%s]fiber-%ld-%d: %s", __func__, fiber->getThread()->thread_id(), fiber->get_id(), strReq.c_str());

    return fiber->sendMsg(ConstUtils::kCmdAuthAck, jsOutput);
}

int login_handler(const NetPkgHeader&, MasterWSFiber* ws,
    const void* pData, const unsigned int uiLen)
{
    const std::string strReq((char*)pData, uiLen);

    Json::Value jsInput;
    Json::Reader reader(Json::Features::strictMode());
    if (!reader.parse(strReq, jsInput)) {
        acl_msg_error("[%s]fiber-%ld-%d: %s", __func__, ws->getThread()->thread_id(), ws->get_id(), strReq.c_str());
        return ws->sendMsg(ConstUtils::kCmdAuthAck, ConstUtils::kErrBodyParse);
    }
    if (jsInput["member_id"].isInt64()) {
        ws->setLoginInfo(jsInput["member_id"].asInt64());
    }

    Json::Value jsOutput;
    if (ws->getThread()->getConnMgr()->login(strReq, ws, jsOutput)) {
        acl_msg_error("[%s]fiber-%ld-%d: %s", __func__, ws->getThread()->thread_id(), ws->get_id(), strReq.c_str());
        return ws->sendMsg(ConstUtils::kCmdAuthAck, ConstUtils::kErrSign);
    }
    jsOutput["code"] = ConstUtils::kSeccuss;
    acl_msg_info("[%s]fiber-%ld-%d: %s", __func__, ws->getThread()->thread_id(), ws->get_id(), strReq.c_str());

    return ws->sendMsg(ConstUtils::kCmdAuthAck, jsOutput);
}

bool logout_handler(MasterHttpServlet* servlet,
    acl::HttpServletRequest& req, acl::HttpServletResponse& res)
{
    const char* szMemId = req.getParameter("member_id");
    const char* szChnId = req.getParameter("channel");
    const char* szUType = req.getParameter("type");

    const char* szSession = req.getParameter("session");
    const char* szThreadId = req.getParameter("thread_no");
    const char* szMsgBody = req.getParameter("msg_body");
    if (szMemId == NULL || szChnId == NULL || szUType == NULL
        || szSession == NULL || szThreadId == NULL) {
        acl_msg_error("[%s]-%s", __func__, req.getQueryString());
        return servlet->responseCode(ConstUtils::kErrParam, res);;
    }
    acl_msg_info("[%s]-%s", __func__, req.getQueryString());

    MPostMsg stMsg;
    do {
        stMsg.command = CMD_LOGOUT_ID;
        stMsg.member_id = (uint32_t)strtoul(szMemId, NULL, 0);
        stMsg.chn = (uint32_t)strtoul(szChnId, NULL, 0);
        stMsg.type = (uint32_t)strtoul(szUType, NULL, 0);
        stMsg.setString(szSession);
        if (szMsgBody != NULL)
            stMsg.setParam2(szMsgBody);
    } while (0);

    push_msg(strtoul(szThreadId, NULL, 0), stMsg);

    return servlet->responseCode(0, res);
}

