#ifndef _MASTER_FIBER_H__
#define _MASTER_FIBER_H__
#include "master_thread.h"
#include "json/json.h"
#include <unordered_map>
#include <functional>

struct FiberNode {
    uint32_t chn, type;
    uint32_t member_id;
    FiberNode(MPostMsg*const ptrMsg) {
        chn = ptrMsg->chn;
        type = ptrMsg->type;
        member_id = ptrMsg->member_id;
    }
    FiberNode(const Json::Value& js) {
        chn = js["channel"].isUInt() ? js["channel"].asUInt() : 0;
        type = js["type"].isUInt() ? js["type"].asUInt() : 0;
        member_id = js["member_id"].isUInt() ? js["member_id"].asUInt() : 0;
    }
    FiberNode(const uint32_t c, const uint32_t t, const uint32_t u) {
        chn = c; type = t; member_id = u;
    }
    bool operator == (const FiberNode &t) const {
        return member_id == t.member_id && chn == t.chn && type == t.type;
    }
};

class MFiberBase : public acl::fiber
{
protected:
    MFiberBase(MasterThread* t, acl::socket_stream* c);
public:
    virtual ~MFiberBase();

public:
    const FiberNode getKey() { return FiberNode(chn_, type_, uiMemberId_); }
    const char* getSession() { return strSession_.c_str(); }

    uint32_t getMemberId() { return uiMemberId_; }
    uint32_t getChannelId() { return chn_; }
    uint32_t getUserType() { return type_; }
    void setLoginInfo(const uint32_t uiMId);
    void setLoginInfo(const FiberNode&, const std::string& strSession);

    inline bool isLogin() { return !strSession_.empty(); }
    inline bool isExit() { return bForceExist_ || client_->eof(); }
    void checkTimeOut(const uint32_t uiNow = 0xffffffff);

public:
    acl::socket_stream* getClient() { return client_; }
    MasterThread* getThread() { return thread_; }
    const char* get_peer_ip()const;

protected:
    acl::socket_stream* client_;
    MasterThread* thread_;
    ACL_FIBER_MUTEX* mutex_;
protected:
    uint32_t chn_, type_;
    uint32_t uiMemberId_;

    bool bForceExist_;
    uint32_t uiCmdSeq_;
    uint32_t uiLastHeart_;
    std::string strSession_;

    std::unordered_map<uint32_t, uint32_t> hash_seq_;
};

#endif // !_MASTER_FIBER_H__
