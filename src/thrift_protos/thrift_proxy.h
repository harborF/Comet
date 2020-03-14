#ifndef _THRIFT_PROXY_H__
#define _THRIFT_PROXY_H__
#include <queue>
#include <unordered_map>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "fiber/lib_fiber.h"
#include "fiber/lib_fiber.hpp"

#include <thrift/transport/TSocketPool.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TMultiplexedProtocol.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

class ThriftPool
{
    typedef std::queue<boost::shared_ptr<TSocket>> queue_socket;
    struct ST_Entry {
        ACL_FIBER_SEM* sem_;
        ACL_FIBER_MUTEX* mutex_;
        queue_socket q_sock_;
        std::string  strHost_;
        uint32_t uiPort_;
    };
    typedef std::unordered_map<uint32_t, ST_Entry*> hash_socket;
public:
    ThriftPool();
    virtual ~ThriftPool();
public:
    void clear();
    void reload();
    boost::shared_ptr<TSocket> getSocket(const uint32_t);
    void releaseSocket(const uint32_t, const boost::shared_ptr<TSocket>&);
private:
    void add_config(const uint32_t, const std::string& strHost, const uint32_t uiPort);
    ST_Entry* getCoonPool(const uint32_t);
private:
    uint32_t uiMaxConn_;
    hash_socket sock_pool_;
    std::vector<ST_Entry*> vec_conn_;
    ACL_FIBER_RWLOCK* rwlock_pool_;
};

class ThriftProxy
{
public:
    ThriftProxy(const uint32_t, ThriftPool*const);
    virtual ~ThriftProxy();

public:
    void clear() {
        socket_.reset();
    };
    boost::shared_ptr<TProtocol> getProtocl();
    boost::shared_ptr<TProtocol> getMultiProtocl(const std::string& serviceName);
private:
    const uint32_t uiCmdId_;
    ThriftPool*const pool_;
    boost::shared_ptr<TSocket> socket_;
};

#endif // !_THRIFT_PROXY_H__
