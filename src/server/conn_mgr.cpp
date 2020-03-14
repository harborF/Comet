#include "conn_mgr.h"
#include "master_http.h"

extern MasterConfigVar g_CfgVar;

ConnMgr::ConnMgr(MasterThread* t) :thread_(t), uiCheckCount_(0), uiCheckTime_(0)
{
    reader_ = new Json::Reader(Json::Features::strictMode());
    acl_assert(reader_ != NULL);
    hash_tcp_ = new Hash_Fiber;
    hash_ws_ = new Hash_WebSocket;
    acl_assert(hash_tcp_ != NULL && hash_ws_ != NULL);
}

ConnMgr::~ConnMgr()
{
    delete reader_;
    reader_ = NULL;
    delete hash_tcp_;
    hash_tcp_ = NULL;
    delete hash_ws_;
    hash_ws_ = NULL;
}

int ConnMgr::login(const std::string& strAuthParam, MasterTcpFiber *f, Json::Value& jsOutput)
{
    acl::string strHost;
    strHost.format("%s%s", g_CfgVar.http_host, g_CfgVar.http_port);
    strHost.url_encode(strHost.c_str());

    acl::string strReqUrl;
    strReqUrl.format("/auth?logic_server=%s&logic_thread=%u",
        strHost.c_str(), thread_->get_thread_index());
    const std::string strRes = HttpUtils::getHttp(g_CfgVar.login_host_port, strReqUrl, strAuthParam);
#ifdef USE_THRIFT
    ThriftProxy proxy(ConstUtils::kCmdAuthReq, thread_->getSockPool());
    try {
        StateSvrClient client_1(proxy.getMultiProtocl("StateSvr"));

        LoginResult loginResult;
        ST_SvrInfo info;
        info.strSvrHost = strHost.c_str();
        info.uiThreadIdx = thread_->get_thread_index();

        client_1.login(loginResult, info, strAuthParam);
        acl_msg_info("[%s]>>thrift code:%d id:%d channel:%d type:%d session:%s",
            __func__, loginResult.retCode,
            loginResult.info.member_id, loginResult.info.channel, loginResult.info.type,
            loginResult.info.session.c_str());

    }
    catch (TException& e) {
        proxy.clear();
        acl_msg_error("[%s]>>thrift exception: %s\n", __func__, e.what());
    }
#endif
    Json::Value jsRes;
    if (strRes.empty() || !reader_->parse(strRes, jsRes)) {
        acl_msg_error("[%s]fiber-%ld-%d parse error %s",
            __func__, thread_->thread_id(), f->get_id(), strRes.c_str());
        return -1;
    }

    if (ConstUtils::kSeccuss != jsRes["code"].asInt()) {
        return -1;
    }
    const Json::Value& jResData = jsRes["data"];
    jsOutput["heartbeat_interval"] = jResData["heartbeat_interval"];

    const FiberNode node(jResData);
    Hash_Fiber::iterator itF = hash_tcp_->find(node);
    if (itF != hash_tcp_->end()) {
        MasterTcpFiber* f2 = itF->second;
        hash_tcp_->erase(itF);

        f2->sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrReLogin);
        f2->checkTimeOut();
        acl_msg_info("[%s]fiber-%ld-%d logout-member: %s-%u-%u-%u",
            __func__, thread_->thread_id(), f2->get_id(), f2->getSession(),
            node.chn, node.type, node.member_id);
    }
    f->setLoginInfo(node, jResData["session"].asString());

    acl_msg_info("[%s]fiber-%ld-%d active-member: %s-%u-%u-%u",
        __func__, thread_->thread_id(), f->get_id(), f->getSession(),
        node.chn, node.type, node.member_id);

    hash_tcp_->insert(Hash_Fiber::value_type(node, f));

    return 0;
}

void ConnMgr::login(MasterTcpFiber *f)
{
    for (auto it = tcp_vec_.begin(); it != tcp_vec_.end(); ++it) {
        if (f == *it) {
            acl_msg_error("[%s]fiber-%ld-%d has existed", __func__, thread_->thread_id(), f->get_id());
            return;
        }
    }
    tcp_vec_.push_back(f);
    thread_->fiber_count_inc();
}

void ConnMgr::logout(MasterTcpFiber *f)
{
    for (auto it = tcp_vec_.begin(); it != tcp_vec_.end(); ++it) {
        if (f == *it) {
            tcp_vec_.erase(it);
            break;
        }
    }
    thread_->fiber_count_dec();

    Hash_Fiber::iterator itF = hash_tcp_->find(f->getKey());
    if (itF != hash_tcp_->end()) {
        hash_tcp_->erase(itF);

        if (f->getMemberId()) {
            acl::string strReqUrl;
            strReqUrl.format("/update?member_id=%u&channel=%u&type=%u&session=%s&status_type=2",
                f->getMemberId(), f->getChannelId(), f->getUserType(),
                f->getSession());
            HttpUtils::getHttp(g_CfgVar.login_host_port, strReqUrl);
        }
    }//end if
}

void ConnMgr::check_connect()
{
    const uint32_t uiNow = time(NULL) - g_CfgVar.uiConnTimeout;
    const uint32_t uiCheckEnd = uiCheckCount_ + 2000;
    for (uint32_t i = uiCheckCount_; i < uiCheckEnd && i < tcp_vec_.size(); ++i) {
        tcp_vec_[i]->checkTimeOut(uiNow);
    }

    for (uint32_t i = uiCheckCount_; i < uiCheckEnd && i < ws_vec_.size(); ++i) {
        ws_vec_[i]->checkTimeOut(uiNow);
    }
#define max(a,b) (((a)>(b))?(a):(b))
    uiCheckCount_ = uiCheckEnd > max(tcp_vec_.size(), ws_vec_.size()) ? 0 : uiCheckEnd;

    acl_msg_info("[%s]>>thread-%lu active: %d dead: %d vec: %zu %zu hash: %zu %zu",
        __func__, thread_->thread_id(),
        thread_->get_fiber_count(), acl_fiber_ndead(),
        tcp_vec_.size(), ws_vec_.size(),
        hash_tcp_->size(), hash_ws_->size());
}

void ConnMgr::dump_connect()
{
    for (unsigned int i = 0; i < tcp_vec_.size(); ++i) {
        ACL_FIBER* f = tcp_vec_[i]->get_fiber();
        acl_msg_info("[%s]fiber-%ld-%d status: %d errno: %d alive: %d",
            __func__, thread_->thread_id(),
            acl_fiber_id(f), acl_fiber_status(f), acl_fiber_errno(f),
            tcp_vec_[i]->getClient()->alive());
    }

    for (unsigned int i = 0; i < ws_vec_.size(); ++i) {
        ACL_FIBER* f = ws_vec_[i]->get_fiber();
        acl_msg_info("[%s]fiber-%ld-%d status: %d errno: %d alive: %d",
            __func__, thread_->thread_id(),
            acl_fiber_id(f), acl_fiber_status(f), acl_fiber_errno(f),
            ws_vec_[i]->getClient()->alive());
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
void ConnMgr::login(MasterWSFiber *ws)
{
    for (auto it = ws_vec_.begin(); it != ws_vec_.end(); ++it) {
        if (ws == *it) {
            acl_msg_error("[%s] has existed", __func__);
            return;
        }
    }
    ws_vec_.push_back(ws);
    thread_->fiber_count_inc();
}

int ConnMgr::login(const std::string& strAuthParam, MasterWSFiber *ws, Json::Value& jsOutput)
{
    acl::string strHost;
    strHost.format("%s%s", g_CfgVar.http_host, g_CfgVar.http_port);
    strHost.url_encode(strHost.c_str());

    acl::string strReqUrl;
    strReqUrl.format("/auth?logic_server=%s&logic_thread=%u", strHost.c_str(), thread_->get_thread_index());
    const std::string strRes = HttpUtils::getHttp(g_CfgVar.login_host_port, strReqUrl, strAuthParam);

    Json::Value jsRes;
    if (strRes.empty() || !reader_->parse(strRes, jsRes)) {
        acl_msg_error("parse error %s", strRes.c_str());
        return -1;
    }

    if (ConstUtils::kSeccuss != jsRes["code"].asInt()) {
        return -1;
    }
    const Json::Value& jResData = jsRes["data"];
    jsOutput["heartbeat_interval"] = jResData["heartbeat_interval"];

    const FiberNode node(jResData);
    Hash_WebSocket::iterator itF = hash_ws_->find(node);
    if (itF != hash_ws_->end()) {
        MasterWSFiber* f2 = itF->second;
        hash_ws_->erase(itF);

        f2->sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrReLogin);
        f2->checkTimeOut();
        acl_msg_info("[%s]fiber-%ld-%d logout-member: %s-%u-%u-%u",
            __func__, thread_->thread_id(), f2->get_id(), f2->getSession(),
            node.chn, node.type, node.member_id);
    }
    ws->setLoginInfo(node, jResData["session"].asString());

    acl_msg_info("[%s]fiber-%ld-%d active-member: %s-%u-%u-%u",
        __func__, thread_->thread_id(), ws->get_id(), ws->getSession(),
        node.chn, node.type, node.member_id);

    hash_ws_->insert(Hash_WebSocket::value_type(node, ws));

    return 0;
}

void ConnMgr::logout(MasterWSFiber *ws)
{
    for (auto it = ws_vec_.begin(); it != ws_vec_.end(); ++it) {
        if (ws == *it) {
            ws_vec_.erase(it);
            break;
        }
    }
    thread_->fiber_count_dec();

    Hash_WebSocket::iterator itF = hash_ws_->find(ws->getKey());
    if (itF != hash_ws_->end()) {
        hash_ws_->erase(itF);

        if (ws->getMemberId()) {
            acl::string strReqUrl;
            strReqUrl.format("/update?member_id=%u&channel=%u&type=%u&session=%s&status_type=2",
                ws->getMemberId(), ws->getChannelId(), ws->getUserType(),
                ws->getSession());
            HttpUtils::getHttp(g_CfgVar.login_host_port, strReqUrl);
        }
    }//end if
}

///////////////////////////////////////////////////////////////////////////////////////////
void ConnMgr::handle_msg(MPostMsg *const ptrMsg)
{
    acl_msg_info("[%s][%u-%u-%u-%u][%u]%s %s", __func__,
        thread_->get_thread_index(), ptrMsg->member_id, ptrMsg->chn, ptrMsg->type,
        ptrMsg->command, ptrMsg->getString(), ptrMsg->getParam2());

    const FiberNode node(ptrMsg);
    if (CMD_LOGOUT_ID == ptrMsg->command) {
        const acl::string session(ptrMsg->getString());
        Hash_Fiber::iterator itF = hash_tcp_->find(node);
        if (itF != hash_tcp_->end()) {
            MasterTcpFiber* f = itF->second;
            if (0 == session.compare(f->getSession())) {
                if (ptrMsg->getParam2() != NULL) {
                    f->sendMsg(ConstUtils::kCmdKillConnReq, std::string(ptrMsg->getParam2()));
                }
                else {
                    f->sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrReLogin);
                }
                f->checkTimeOut();
                acl_msg_info("[%s]fiber-%ld-%d logout-member: %s-%u-%u-%u",
                    __func__, thread_->thread_id(), f->get_id(), f->getSession(),
                    node.chn, node.type, node.member_id);
            }
        }//end if

        Hash_WebSocket::iterator itF2 = hash_ws_->find(node);
        if (itF2 != hash_ws_->end()) {
            MasterWSFiber* f = itF2->second;
            if (0 == session.compare(f->getSession())) {
                if (ptrMsg->getParam2() != NULL) {
                    f->sendMsg(ConstUtils::kCmdKillConnReq, std::string(ptrMsg->getParam2()));
                }
                else {
                    f->sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrReLogin);
                }
                f->checkTimeOut();
                acl_msg_info("[%s]fiber-%ld-%d logout-member: %s-%u-%u-%u",
                    __func__, thread_->thread_id(), f->get_id(), f->getSession(),
                    node.chn, node.type, node.member_id);
            }
        }//end if
    }
    else {
        post_msg(node, ConstUtils::kCmdPushMsgReq, ptrMsg->getString());
    }//end if
}

int ConnMgr::post_msg(const FiberNode& node, const uint32_t cmdId, const acl::string& aclMsg)
{
    Hash_Fiber::iterator itF = hash_tcp_->find(node);
    if (itF != hash_tcp_->end()) {
        return itF->second->sendMsg(cmdId, std::string(aclMsg.c_str()));
    }

    Hash_WebSocket::iterator itF2 = hash_ws_->find(node);
    if (itF2 != hash_ws_->end()) {
        return itF2->second->sendMsg(cmdId, aclMsg.c_str());
    }

    acl_msg_error("[%s][%u]push message error", __func__, thread_->get_thread_index());
    return -1;
}

