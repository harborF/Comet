#ifndef _CONN_MGR_H__
#define _CONN_MGR_H__
#include "master_tcp.h"
#include "master_ws.h"

struct FiberNodeHash {
    static inline void hash_combine(size_t& seed, size_t hv) {
        seed ^= hv + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    std::size_t operator () (const FiberNode &t) const {
        size_t seed = 0;    
        hash_combine(seed, std::hash<uint32_t>{}(t.member_id));
        hash_combine(seed, std::hash<uint32_t>{}(t.chn));    
        hash_combine(seed, std::hash<uint32_t>{}(t.type));    
        return seed;
    }
};

class ConnMgr
{
    typedef std::unordered_map<FiberNode, MasterTcpFiber*, FiberNodeHash> Hash_Fiber;
    typedef std::unordered_map<FiberNode, MasterWSFiber*, FiberNodeHash> Hash_WebSocket;
public:
    ConnMgr(MasterThread* t);
    ~ConnMgr();

public:
    void login(MasterTcpFiber *f);
    int login(const std::string&, MasterTcpFiber *f, Json::Value&);
    void logout(MasterTcpFiber *f);

    void login(MasterWSFiber *f);
    int login(const std::string&, MasterWSFiber *f, Json::Value&);
    void logout(MasterWSFiber *f);

    void check_connect();
    void dump_connect();
    
    void handle_msg(MPostMsg *const);
    int post_msg(const FiberNode& node, const uint32_t cmdId, const acl::string&);
private:
    MasterThread* thread_;
    Json::Reader* reader_;
    
    Hash_Fiber* hash_tcp_;
    Hash_WebSocket* hash_ws_;

    uint32_t uiCheckCount_, uiCheckTime_;
    std::vector<MasterTcpFiber*> tcp_vec_;
    std::vector<MasterWSFiber*> ws_vec_;
};

#endif // !_CONN_MGR_H__
