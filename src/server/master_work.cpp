#include "master_work.h"

extern MasterConfigVar g_CfgVar;

MasterWork::MasterWork(MasterThread* t) :uiIdx_(0), thread_(t)
{
}

MasterWork::~MasterWork()
{
}

void MasterWork::erase_channel(ACL_CHANNEL* chn)
{
    for (auto it = vec_channel_.begin(); it != vec_channel_.end(); ++it) {
        if (*it == chn) {
            vec_channel_.erase(it);
            break;
        }
    }//end for
}

void MasterWork::fiber_callback(ACL_FIBER *f, void *ctx)
{
    MasterWork*pThis = (MasterWork*)ctx;
    ACL_CHANNEL* channel = acl_channel_create(sizeof(unsigned long), 1024);
    acl_assert(channel != NULL);
    pThis->vec_channel_.push_back(channel);

    for (;;) {
        void* ret = acl_channel_recvp(channel);
        if (ret == NULL) {
            acl_msg_error("[%s]fiber-%ld-%d: acl_channel_recvp error",
                __func__, pThis->getThread()->thread_id(), acl_fiber_id(f));
            break;
        }

#ifdef USE_THRIFT
        ThriftProxy proxy(stHdr.uiCmdId, pThis->getThread()->getSockPool());
        try {
            StateSvrClient client(proxy.getProtocl());

            ST_MsgResult msgRet;
            client.handleRequest(msgRet, 100, strAuthParam);
            acl_msg_info("[%s]>>thrift command:100 ret code:%d message:%s",
                __func__, msgRet.retCode, msgRet.jsData.c_str());

        }
        catch (TException& e) {
            proxy.clear();
            acl_msg_error("[%s]>>thrift exception: %s\n", __func__, e.what());
        }
#endif

    }//end for

    pThis->erase_channel(channel);
    acl_channel_free(channel);
}

void MasterWork::start_works(const uint32_t uiCount)
{
    for (uint32_t i = 0; i < uiCount; ++i) {
        acl_fiber_create(fiber_callback, this, g_CfgVar.uiFiberStackSize);
    }
}

void MasterWork::transfer_message()
{
    if (vec_channel_.size()) {
        acl_channel_sendp(vec_channel_[uiIdx_++ % vec_channel_.size()], NULL);
    }
}
