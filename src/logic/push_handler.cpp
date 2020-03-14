#include "opr_handler.h"

extern MasterConfigVar g_CfgVar;

bool push_handler(MasterHttpServlet* servlet,
    acl::HttpServletRequest& req, acl::HttpServletResponse& res)
{
    const char* szMemId = req.getParameter("member_id");
    const char* szChnId = req.getParameter("channel");
    const char* szUType = req.getParameter("type");

    const char* szThrIdx = req.getParameter("logic_thread");
    const char* szMsgBody = req.getParameter("msg_body");

    acl_msg_info("[%s] member:%s channel:%s type:%s thread:%s",
        __func__,
        szMemId, szChnId, szUType, szThrIdx);

    if (szMemId == NULL || szChnId == NULL || szUType == NULL
        || szThrIdx == NULL || szMsgBody == NULL) {
        return servlet->responseCode(ConstUtils::kErrParam, res);
    }
    else {
        MPostMsg stMsg;
        do {
            stMsg.command = CMD_POST_ID;
            stMsg.member_id = (uint32_t)strtoul(szMemId, NULL, 0);
            stMsg.chn = (uint32_t)strtoul(szChnId, NULL, 0);
            stMsg.type = (uint32_t)strtoul(szUType, NULL, 0);
            stMsg.setString(szMsgBody);
        } while (0);

        push_msg(strtoul(szThrIdx, NULL, 0), stMsg);
    }

    return servlet->responseCode(ConstUtils::kSeccuss, res);
}

int push_ack_handler(const NetPkgHeader&, MasterTcpFiber* fiber,
    const void* pData, const unsigned int uiLen)
{
    const std::string strReq((char*)pData, uiLen);
    acl_msg_info("[%s]fiber-%ld-%d: member-%u %s", 
        __func__, fiber->getThread()->thread_id(), fiber->get_id(),
        fiber->getMemberId(), strReq.c_str());

    Json::Value jsInput;
    Json::Reader reader(Json::Features::strictMode());
    if (!reader.parse(strReq, jsInput)) {
        return -1;
    }

    if (!jsInput["code"].isInt() || jsInput["code"].asInt()) {
        acl_msg_error("[%s]fiber-%ld-%d: member-%u %s",
            __func__, fiber->getThread()->thread_id(), fiber->get_id(),
            fiber->getMemberId(), strReq.c_str());
        return -2;
    }
    const Json::Value& jsData = jsInput["data"];
    if (!jsData["num"].isUInt() || !jsData["packageid"].isString()) {
        acl_msg_error("[%s]fiber-%ld-%d: member-%u %s",
            __func__, fiber->getThread()->thread_id(), fiber->get_id(),
            fiber->getMemberId(), strReq.c_str());
        return -3;
    }

    acl::string strReqUrl;
    strReqUrl.format("/delete_msg?id=%u&type=%u&channel=%u&num=%d&packageid=%s",
        fiber->getMemberId(), fiber->getUserType(), fiber->getChannelId(),
        jsData["num"].asUInt(),
        jsData["packageid"].asCString());
    HttpUtils::getHttp(g_CfgVar.msg_host_port, strReqUrl);

    return 0;
}

int push_ack_handler(const NetPkgHeader&, MasterWSFiber* ws,
    const void* pData, const unsigned int uiLen)
{
    const std::string strReq((char*)pData, uiLen);
    acl_msg_info("[%s]fiber-%ld-%d: member-%u %s", 
        __func__, ws->getThread()->thread_id(), ws->get_id(),
        ws->getMemberId(),strReq.c_str());

    Json::Value jsInput;
    Json::Reader reader(Json::Features::strictMode());
    if (!reader.parse(strReq, jsInput)) {
        return -1;
    }

    if (!jsInput["code"].isInt() || jsInput["code"].asInt()) {
        acl_msg_error("[%s]fiber-%ld-%d: member-%u %s",
            __func__, ws->getThread()->thread_id(), ws->get_id(),
            ws->getMemberId(), strReq.c_str());
        return -2;
    }
    const Json::Value& jsData = jsInput["data"];
    if (!jsData["num"].isUInt() || !jsData["packageid"].isString()) {
        acl_msg_error("[%s]fiber-%ld-%d: member-%u %s",
            __func__, ws->getThread()->thread_id(), ws->get_id(),
            ws->getMemberId(), strReq.c_str());
        return -3;
    }

    acl::string strReqUrl;
    strReqUrl.format("/delete_msg?id=%u&type=%u&channel=%u&num=%d&packageid=%s",
        ws->getMemberId(), ws->getUserType(), ws->getChannelId(),
        jsData["num"].asInt(),
        jsData["packageid"].asCString());
    HttpUtils::getHttp(g_CfgVar.msg_host_port, strReqUrl);

    return 0;
}

