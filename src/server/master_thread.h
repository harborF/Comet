#ifndef _MASTER_THREAD_H__
#define _MASTER_THREAD_H__
#ifdef USE_THRIFT
#include "thrift_proxy.h"
#include "StateSvr.h"
using namespace  ::protruly;
#endif
#include "master_config.h"
#include "fiber/lib_fiber.h"
#include "fiber/lib_fiber.hpp"

#define CMD_TCP_ID 'c'
#define CMD_HTTP_ID 'h'
#define CMD_WS_ID 'w'
#define CMD_TIMEOUT_ID 't'
#define CMD_DUMP_ID 'd'

#define CMD_LOGOUT_ID 110
#define CMD_POST_ID 111
#define CMD_RELOAD_ID 112

class MPostMsg
{
public:
    MPostMsg() :chn(0), type(0), member_id(0), command(0), ptrParam(NULL), msgBuf_(NULL), msgBuf2_(NULL) {

    }
    MPostMsg(uint32_t cmd, void*const ptr) :chn(0), type(0), member_id(0)
        , command(cmd), ptrParam(ptr), msgBuf_(NULL), msgBuf2_(NULL) {

    }
    MPostMsg(const MPostMsg& o) :ptrParam(o.ptrParam) {
        chn = o.chn; type = o.type;
        member_id = o.member_id;
        command = o.command;
        msgBuf_ = o.msgBuf_ ? acl_mystrdup(o.msgBuf_) : NULL;
        msgBuf2_ = o.msgBuf2_ ? acl_mystrdup(o.msgBuf2_) : NULL;
    }
    ~MPostMsg() {
        if (msgBuf_) acl_myfree(msgBuf_);
        if (msgBuf2_) acl_myfree(msgBuf2_);
    }

    void setString(const char* sz) {
        if (msgBuf_) acl_myfree(msgBuf_);
        msgBuf_ = acl_mystrdup(sz);
    }
    const char* getString() { return msgBuf_; }

    void setParam2(const char* sz) {
        if (msgBuf2_) acl_myfree(msgBuf2_);
        msgBuf2_ = acl_mystrdup(sz);
    }
    const char* getParam2() { return msgBuf2_; }

    static void free(void *ctx) {
        MPostMsg *ptr = (MPostMsg *)ctx;
        if (ptr->ptrParam) {
            delete (acl::socket_stream *)ptr->ptrParam;
        }
        if (ptr->msgBuf_) acl_myfree(ptr->msgBuf_);
        if (ptr->msgBuf2_) acl_myfree(ptr->msgBuf2_);
        delete ptr;
    }
public:
    uint32_t chn, type;
    uint32_t member_id;
    uint32_t command;
    const void* ptrParam;
private:
    char* msgBuf_;
    char* msgBuf2_;
};

int push_msg(const uint32_t uiTid, const MPostMsg& szMsg);

class ConnMgr;
class MasterThread : public acl::thread
{
public:
    MasterThread(unsigned int thread_index, ACL_MBOX*);

    ~MasterThread(void);

public:
    int get_fiber_count(void) { return fibers_cnt_; }
    void fiber_count_inc(void) { ++fibers_cnt_; }
    void fiber_count_dec(void) { --fibers_cnt_; }

    const uint32_t get_thread_index() {
        return thread_index_;
    }
public:
#ifdef USE_REDIS
    ACL_FIBER_SEM* get_redis_sem() { return sem_redis_; }
    acl::redis_client_cluster& get_redis() { return cluster_; }
#endif
    ConnMgr* getConnMgr() { return conn_mgr_; }
#ifdef USE_THRIFT
    ThriftPool* getSockPool() { return conn_pool_; }
#endif
    ACL_FIBER_MUTEX* getMutex() { return mutex_; }
protected:
    void *run(void);

    static void fiber_callback(ACL_FIBER *f, void *ctx);
private:
    int fibers_cnt_;
    const uint32_t thread_index_;

    ACL_MBOX* mbox_;
    ACL_FIBER_MUTEX* mutex_;
    ACL_CHANNEL* chn_notify_;
#ifdef USE_REDIS
    ACL_FIBER_SEM* sem_redis_;
    acl::redis_client_cluster cluster_;
#endif
#ifdef USE_THRIFT
    ThriftPool* conn_pool_;
#endif
    ConnMgr* conn_mgr_;
};

#endif // !_MASTER_THREAD_H__
