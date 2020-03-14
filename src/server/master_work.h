#ifndef _MASTER_WORK_H__
#define _MASTER_WORK_H__
#include "master_thread.h"
#include "json/json.h"

class MasterWork
{
public:
    MasterWork(MasterThread* t);
    virtual ~MasterWork();

protected:
    static void fiber_callback(ACL_FIBER *f, void *ctx);
    void erase_channel(ACL_CHANNEL*);

public:
    void start_works(const uint32_t);
    void transfer_message();
    MasterThread* getThread() { return thread_; }

private:
    uint32_t uiIdx_;
    MasterThread* thread_;
    std::vector<ACL_CHANNEL*> vec_channel_;
};

#endif
