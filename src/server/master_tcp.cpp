#include "master_tcp.h"
#include "conn_mgr.h"

extern MasterConfigVar g_CfgVar;

MasterTcpFiber::MasterTcpFiber(MasterThread* t, acl::socket_stream* c)
    :MFiberBase(t, c)
{
    thread_->getConnMgr()->login(this);
    buffer_ = acl_mymalloc(g_CfgVar.uiMaxReadBuffer);
    acl_assert(buffer_ != NULL);
}

MasterTcpFiber::~MasterTcpFiber(void)
{
    thread_->getConnMgr()->logout(this);
    acl_myfree(buffer_);
    delete client_;
}

void MasterTcpFiber::run(void)
{
    acl_msg_info("[%s]fiber-%ld-%d fd %d peer %s",
        __func__, thread_->thread_id(), get_id(),
        client_->sock_handle(), client_->get_peer());

    int ret = 0;
    NetPkgHeader stHeader;
    while (!isExit())
    {
        ret = client_->read(buffer_, sizeof(NetPkgHeader));
        if (ret != sizeof(NetPkgHeader) || isExit()) {
            break;
        }
        CommUtils::readNetHeader(buffer_, stHeader);

        const uint32_t uiDataLen = stHeader.uiPkgLen - stHeader.usHdrLen;
        if (uiDataLen >= g_CfgVar.uiMaxReadBuffer) {
            acl_msg_error("[%s]fiber-%ld-%d Failed to get: %d, fd: %d data len: %u",
                __func__, thread_->thread_id(), get_id(),
                get_errno(), client_->sock_handle(), uiDataLen);
            sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrLength);
            break;
        }
        else if (uiDataLen) {
            ret = client_->read(buffer_, uiDataLen);
            if (ret != uiDataLen || isExit()) {
                acl_msg_error("[%s]fiber-%ld-%d Failed to get: %d, fd: %d",
                    __func__, thread_->thread_id(), get_id(),
                    get_errno(), client_->sock_handle());
                break;
            }
        }
        else {
            ret = 0;
        }

        OprHandler handler = getHandler(stHeader.uiCmdId);
        if (NULL == handler) {
            acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u",
                __func__, thread_->thread_id(), get_id(),
                this->getMemberId(), stHeader.uiCmdId);
            sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrCmdId);
            break;
        }

        if (!this->isLogin() && stHeader.uiCmdId > ConstUtils::kCmdAuthReq) {
            acl_msg_error("[%s]fiber-%ld-%d command id:%u not login",
                __func__, thread_->thread_id(), get_id(), stHeader.uiCmdId);
            sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrNoLogin);
            break;
        }
        ++uiCmdSeq_;
        uiLastHeart_ = time(NULL);
#if USE_SUB_FIBER
        MasterSubTcpFiber* subF = new MasterSubTcpFiber(this, buffer_, ret);
        subF->handler_ = handler;
        subF->stHeader_ = stHeader;
        subF->start();
#else
        try {
            if (-1 == handler(stHeader, this, buffer_, ret)) {
                acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u ret error",
                    __func__, thread_->thread_id(), get_id(),
                    this->getMemberId(), stHeader.uiCmdId);
                break;
            }
        }
        catch (...) {
            acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u exception",
                __func__, thread_->thread_id(), get_id(),
                this->getMemberId(), stHeader.uiCmdId);
        }
#endif
    }//end while

    acl_msg_info("[%s]fiber-%ld-%d force-%d close tcp", __func__, thread_->thread_id(), get_id(), bForceExist_);

    delete this;
}

int MasterTcpFiber::sendMsg(const uint32_t uiCmdId, const Json::Value& jsVal)
{
    Json::FastWriter writer;
    const std::string str = writer.write(jsVal);
    return sendMsg(uiCmdId, str.c_str(), str.length());
}

int MasterTcpFiber::sendMsg(const uint32_t uiCmdId, const int code)
{
    acl::string str;
    str.format("{\"code\":%d}", code);
    return sendMsg(uiCmdId, str.c_str(), str.length());
}

int MasterTcpFiber::sendMsg(const uint32_t uiCmdId, const std::string& strMsg)
{
    return sendMsg(uiCmdId, strMsg.c_str(), strMsg.length());
}

int MasterTcpFiber::sendMsg(const uint32_t uiCmdId, const void* msg, const uint32_t uiLen)
{
    if (isExit()) {
        acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u",
            __func__, thread_->thread_id(), get_id(), uiMemberId_, uiCmdId);
        return -1;
    }

    NetPkgHeader stHeader;
    stHeader.usHdrLen = sizeof(NetPkgHeader);
    stHeader.uiCmdId = uiCmdId;
    stHeader.uiSeqId = ++uiCmdSeq_;
    stHeader.uiPkgLen = stHeader.usHdrLen + uiLen;

    acl_fiber_mutex_lock(mutex_);

    CommUtils::writeNetHeader(buffer_, stHeader);
    if (client_->write(buffer_, sizeof(NetPkgHeader)) != sizeof(NetPkgHeader)) {
		acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u write error",
			__func__, thread_->thread_id(), get_id(),
			uiMemberId_, uiCmdId);
		this->checkTimeOut();

        acl_fiber_mutex_unlock(mutex_);

        acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u",
            __func__, thread_->thread_id(), get_id(), uiMemberId_, uiCmdId);
        return -1;
    }

	if (isExit()) {
		acl_fiber_mutex_unlock(mutex_);

        acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u",
            __func__, thread_->thread_id(), get_id(), uiMemberId_, uiCmdId);
        return -1;
	}

    if (uiLen > 0 && client_->write(msg, uiLen) != uiLen) {
		acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u write error",
			__func__, thread_->thread_id(), get_id(),
			uiMemberId_, uiCmdId);
		this->checkTimeOut();

        acl_fiber_mutex_unlock(mutex_);
        acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u",
            __func__, thread_->thread_id(), get_id(), uiMemberId_, uiCmdId);

        return -1;
    }
    acl_fiber_mutex_unlock(mutex_);

    acl_msg_info("[%s]fiber-%ld-%d member-%u command-%u",
        __func__, thread_->thread_id(), get_id(), uiMemberId_, uiCmdId);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
MasterSubTcpFiber::MasterSubTcpFiber(MasterTcpFiber* f, void* buf, int len) :
    tcp_fiber_(f), bufLen_(len)
{
    buffer_ = acl_mymalloc(len + 1);
    acl_assert(buffer_ != NULL);
    memcpy(buffer_, buf, len);
}

MasterSubTcpFiber::~MasterSubTcpFiber()
{
    acl_myfree(buffer_);
}

void MasterSubTcpFiber::run(void)
{
    try {
        if (-1 == handler_(stHeader_, tcp_fiber_, buffer_, bufLen_)) {
            acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u ret error",
                __func__, tcp_fiber_->getThread()->thread_id(), tcp_fiber_->get_id(),
                tcp_fiber_->getMemberId(), stHeader_.uiCmdId);
        }
    }
    catch (...) {
        acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u exception",
            __func__, tcp_fiber_->getThread()->thread_id(), tcp_fiber_->get_id(),
            tcp_fiber_->getMemberId(), stHeader_.uiCmdId);
    }

    delete this;
}
