#include "client_thread.h"
#include "client_fiber.h"

#define FIBER_STACK_SIZE (8*1024)
static unsigned int s_uiActiveFiber = 0;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

ClientThread::ClientThread(int fibers_max, unsigned int stack_size,
    ACL_MBOX* mbox, acl::string& addr)
    : fibers_max_(fibers_max)
    , fibers_cnt_(0)
    , stack_size_(stack_size)
    , mbox_(mbox)
    , strServerAddr_(addr)
{

}

ClientThread::~ClientThread(void)
{

}

void ClientThread::fiber_callback(ACL_FIBER *f, void *ctx)
{
    ClientThread* me = (ClientThread *)ctx;

    for (;;) {
        MPostMsg *ptrMsg = (MPostMsg*)acl_mbox_read(me->mbox_, 0, NULL);
        if (ptrMsg == NULL)
            continue;

        if (ptrMsg->command == 'c') {
            if (me->fibers_cnt_ < me->fibers_max_) {
                const uint32_t iCount = (me->fibers_max_ - me->fibers_cnt_) % 500 + 100;
                for (uint32_t i = 0; i < iCount; ++i) {
                    (new ClientFiber(me))->start(FIBER_STACK_SIZE);
                }
            }
        }
        else if (ptrMsg->command == 't') {
            for (auto it = me->login_vec_.begin(); it != me->login_vec_.end(); ++it) {
                ClientFiber *fiber = *it;
                fiber->sendMsg(ConstUtils::kCmdHeartReq, "");
            }
        }
        acl_msg_info(">>thread %lu fibers: %d %d",
            me->thread_id(), me->fibers_max_, me->fibers_cnt_);
        delete ptrMsg;
    }
}

void* ClientThread::run(void)
{
    gettimeofday(&begin_, NULL);

    acl_fiber_create(fiber_callback, this, stack_size_);

    acl_fiber_schedule();

    return NULL;
}

void ClientThread::login(ClientFiber *f)
{
    ++fibers_cnt_;
    login_vec_.push_back(f);

    pthread_mutex_lock(&g_mutex);
    ++s_uiActiveFiber;
    pthread_mutex_unlock(&g_mutex);
}

void ClientThread::logout(ClientFiber *f)
{
    for (auto it = login_vec_.begin(); it != login_vec_.end(); ++it) {
        if (f == *it) {
            login_vec_.erase(it);
            break;
        }
    }

    pthread_mutex_lock(&g_mutex);
    --s_uiActiveFiber;
    pthread_mutex_unlock(&g_mutex);

    if (--fibers_cnt_ > 0)
        return;

    pthread_mutex_lock(&g_mutex);
    if (s_uiActiveFiber == 0)
        exit(0);
    pthread_mutex_unlock(&g_mutex);
}

