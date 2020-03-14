#include "master_thread.h"
#include "master_tcp.h"
#include "master_http.h"
#include "master_ws.h"
#include "conn_mgr.h"

extern MasterConfigVar g_CfgVar;

MasterThread::MasterThread(unsigned int thread_index, ACL_MBOX* mbox)
    : fibers_cnt_(0)
    , thread_index_(thread_index)
    , mbox_(mbox)
{
    conn_mgr_ = new ConnMgr(this);
    acl_assert(conn_mgr_ != NULL);
}

MasterThread::~MasterThread(void)
{
    delete conn_mgr_;
    acl_mbox_free(mbox_, MPostMsg::free);
}

void MasterThread::fiber_callback(ACL_FIBER *f, void *ctx)
{
    MasterThread* me = (MasterThread *)ctx;

#ifdef USE_REDIS
    me->sem_redis_ = acl_fiber_sem_create(10);
#endif
#ifdef USE_THRIFT
    me->conn_pool_ = new ThriftPool();
    acl_assert(me->conn_pool_ != NULL);
#endif
    me->mutex_ = acl_fiber_mutex_create();
    for (;;) {
        MPostMsg *ptrMsg = (MPostMsg*)acl_mbox_read(me->mbox_, 0, NULL);
        if (ptrMsg == NULL)
            continue;

        switch (ptrMsg->command)
        {
        case CMD_TCP_ID:
        {
            acl::socket_stream *client = (acl::socket_stream *)ptrMsg->ptrParam;
            if (g_CfgVar.uiMaxFibers > me->fibers_cnt_) {
                (new MasterTcpFiber(me, client))->start(g_CfgVar.uiFiberStackSize);
            }
            else {
                client->close();
                delete client;
            }
        }
        break;
        case CMD_TIMEOUT_ID:
            me->getConnMgr()->check_connect();
            break;
        case CMD_DUMP_ID:
            me->getConnMgr()->dump_connect();
            break;
        case CMD_LOGOUT_ID:
        case CMD_POST_ID:
            me->getConnMgr()->handle_msg(ptrMsg);
            break;
        case CMD_HTTP_ID:
        {
            acl::socket_stream *client = (acl::socket_stream *)ptrMsg->ptrParam;
            (new MasterHttpFiber(me, client))->start(g_CfgVar.uiFiberStackSize);
        }
        break;
        case CMD_WS_ID:
        {
            acl::socket_stream *client = (acl::socket_stream *)ptrMsg->ptrParam;
            if (g_CfgVar.uiMaxFibers > me->fibers_cnt_) {
                (new MasterWSFiber(me, client))->start(g_CfgVar.uiFiberStackSize);
            }
            else {
                client->close();
                delete client;
            }
        }
        break;
        default:
            acl_msg_fatal("[%s]>>thread %lu command: %u",
                __func__, me->thread_id(), ptrMsg->command);
            break;
        }
        delete ptrMsg;
    }
#ifdef USE_REDIS
    acl_fiber_sem_free(me->sem_redis_);
#endif
    acl_fiber_mutex_free(me->mutex_);
}

void* MasterThread::run(void)
{
#ifdef USE_REDIS
    acl_msg_info(">>redis addr: %s", g_CfgVar.redis_host_port);
    cluster_.set(g_CfgVar.redis_host_port, 10, 2, 2);
#endif
    acl_fiber_create(fiber_callback, this, g_CfgVar.uiFiberStackSize);

    acl_fiber_schedule();

    return NULL;
}
