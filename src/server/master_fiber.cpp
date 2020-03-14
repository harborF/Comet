#include "master_fiber.h"

extern MasterConfigVar g_CfgVar;

MFiberBase::MFiberBase(MasterThread* t, acl::socket_stream* c) :
    client_(c), thread_(t),
    chn_(0), type_(0), uiMemberId_(0),
    uiCmdSeq_(0), bForceExist_(false)
{
    uiLastHeart_ = time(NULL);
    mutex_ = acl_fiber_mutex_create();
}

MFiberBase::~MFiberBase()
{
    acl_fiber_mutex_free(mutex_);
    if (0 == thread_->get_fiber_count() % 100)
        acl_msg_info("[%s]>>thread %lu active-fibers: %d %d",
            __func__, thread_->thread_id(), thread_->get_fiber_count(), acl_fiber_ndead());
}

const char* MFiberBase::get_peer_ip()const
{
    return client_->get_peer_ip();
}

void MFiberBase::setLoginInfo(const uint32_t uiMId)
{
    if (uiMemberId_ != 0 && uiMemberId_ != uiMId) {
        acl_msg_error("[%s]fiber-%ld-%d member error", __func__, thread_->thread_id(), get_id());
    }
    uiMemberId_ = uiMId;
}

void MFiberBase::setLoginInfo(const FiberNode& node, const std::string& strSession)
{
    if (strSession.empty()) {
        acl_msg_error("[%s]fiber-%ld-%d member error", __func__, thread_->thread_id(), get_id());
    }

    chn_ = node.chn;
    type_ = node.type;
    uiMemberId_ = node.member_id;
    strSession_ = strSession;
}

void MFiberBase::checkTimeOut(const uint32_t uiNow)
{
    if (bForceExist_ && uiLastHeart_ + (g_CfgVar.uiConnTimeout << 1) < uiNow) {
        acl_msg_info("[%s]fiber-%ld-%d notify kill", __func__, thread_->thread_id(), get_id());

        this->uiLastHeart_ = uiNow;
        if(0 == acl_fiber_killed(get_fiber()))
            acl_fiber_kill(get_fiber());
    }
    else if (!bForceExist_ && uiLastHeart_ < uiNow) {
        bForceExist_ = true;
        client_->close();
        acl_msg_info("[%s]fiber-%ld-%d last %u < %u force exit",
            __func__, thread_->thread_id(), get_id(),
            uiLastHeart_, uiNow);
    }
}
