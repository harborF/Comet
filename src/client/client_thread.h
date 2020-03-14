#ifndef _CLIENT_THREAD_H__
#define _CLIENT_THREAD_H__
#include "fiber/lib_fiber.h"
#include "fiber/lib_fiber.hpp"
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "common/CmdDefine.h"
#include "common/CommUtils.h"

class ClientFiber;
class MPostMsg
{
public:
    MPostMsg():command(0),ptrParam(NULL) {

    }
    MPostMsg(uint32_t cmd, void*const ptr):command(cmd),ptrParam(ptr) {
    }
    MPostMsg(const MPostMsg& o):ptrParam(o.ptrParam) {
        command = o.command;
    }
public:
    uint32_t command;
    const void* ptrParam;
};

class ClientThread : public acl::thread
{
public:
    ClientThread(int fibers_max, unsigned int stack_size,
        ACL_MBOX*, acl::string& addr);

    ~ClientThread(void);

public:
    int get_fibers_max(void) const {
        return fibers_max_;
    }
    int get_fiber_count(void) const {
        return fibers_cnt_;
    }
    struct timeval& get_begin(void) {
        return begin_;
    }
    const char* get_server_addr(void) {
        return strServerAddr_.c_str();
    }
public:
    void login(ClientFiber *f);
    void logout(ClientFiber *f);
protected:
    void *run(void);

    static void fiber_callback(ACL_FIBER *f, void *ctx);
private:
    int fibers_max_, fibers_cnt_;
    const unsigned int stack_size_;
    struct timeval begin_;
    acl::string strServerAddr_;

    ACL_MBOX* mbox_;
    std::vector<ClientFiber *> login_vec_;
};

#endif // !_MASTER_THREAD_H__
