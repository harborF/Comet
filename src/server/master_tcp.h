#ifndef _MASTER_TCP_H__
#define _MASTER_TCP_H__
#include "master_thread.h"
#include "master_fiber.h"

class MasterTcpFiber : public MFiberBase
{
public:
    MasterTcpFiber(MasterThread* t, acl::socket_stream* c);
    ~MasterTcpFiber(void);
public:
    int sendMsg(const uint32_t uiCmdId, const Json::Value&);
    int sendMsg(const uint32_t uiCmdId, const int);
    int sendMsg(const uint32_t uiCmdId, const std::string&);
    int sendMsg(const uint32_t uiCmdId, const void*, const uint32_t);
protected:
    void run(void);

private:
    void* buffer_;
};

class MasterSubTcpFiber: public acl::fiber
{
    friend class MasterTcpFiber;
public:
    MasterSubTcpFiber(MasterTcpFiber*, void*, int);
    ~MasterSubTcpFiber();

protected:
    void run(void);

private:
    void* buffer_;
    int bufLen_;
    OprHandler handler_;
    NetPkgHeader stHeader_;
    MasterTcpFiber* tcp_fiber_;
};

#endif // !_MASTER_FIBER_H__
