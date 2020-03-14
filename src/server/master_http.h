#ifndef _MASTER_HTTP_H__
#define _MASTER_HTTP_H__
#include "master_thread.h"
#include "json/json.h"

class MasterHttpFiber;
class MasterHttpServlet : public acl::HttpServlet
{
public:
    MasterHttpServlet(MasterHttpFiber* f, acl::socket_stream* stream);

    ~MasterHttpServlet(void);
public:
    MasterThread* getThread();
    bool responseCode(const int, acl::HttpServletResponse& res);
    bool responseJson(Json::Value&, acl::HttpServletResponse& res);
private:
    // override
    bool doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
    bool doPost(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
    bool doError(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
private:
    MasterHttpFiber *fiber_;
};

class MasterHttpFiber : public acl::fiber
{
    friend class MasterHttpServlet;
public:
    MasterHttpFiber(MasterThread* t, acl::socket_stream* c);
    ~MasterHttpFiber(void);

public:
    acl::socket_stream* getClient() {
        return client_;
    }
    MasterThread* getThread() {
        return thread_;
    }
protected:
    void run(void);

private:
    acl::socket_stream* client_;
    MasterThread* thread_;
};

#endif // !_MASTER_HTTP_H__
