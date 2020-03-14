#ifndef _CLIENT_FIBER_H__
#define _CLIENT_FIBER_H__
#include "client_thread.h"
#include "json/json.h"

class ClientFiber : public acl::fiber
{
public:
    ClientFiber(ClientThread* t);
    ~ClientFiber(void);

public:
    bool sendMsg(const unsigned int uiCmdId, const char*);
    void handle_msg(const NetPkgHeader&,const char*);
protected:
    void run(void);

private:
    ACL_VSTREAM* client_;
    ClientThread* thread_;
};

#endif // !_CLIENT_FIBER_H__
