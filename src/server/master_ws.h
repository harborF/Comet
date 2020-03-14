#ifndef _MASTER_WS_H__
#define _MASTER_WS_H__
#include "master_thread.h"
#include "master_fiber.h"
#include "json/json.h"

class MasterWSFiber;
class MasterWSServlet : public acl::HttpServlet
{
public:
    MasterWSServlet(MasterWSFiber*f,acl::socket_stream* stream);
	~MasterWSServlet();

protected:
	// @override
	bool doUnknown(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doGet(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doPost(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doWebsocket(acl::HttpServletRequest&, acl::HttpServletResponse&);

private:
	bool doPing(acl::websocket&, acl::websocket&);
	bool doPong(acl::websocket&, acl::websocket&);
	bool doClose(acl::websocket&, acl::websocket&);

    MasterWSFiber *fiber_;
};

class MasterWSFiber : public MFiberBase
{
    friend class MasterWSServlet;
public:
    MasterWSFiber(MasterThread* t, acl::socket_stream* c);
    ~MasterWSFiber(void);
public:
    bool handleMsg(const char* msg, size_t uiLen);
    int sendMsg(const unsigned int uiCmdId, const Json::Value&);
    int sendMsg(const unsigned int uiCmdId, const int);
    int sendMsg(const unsigned int uiCmdId, const char*);
    int sendMsg(const unsigned int uiCmdId, const void*, const unsigned int);
protected:
    void run(void);

private:
    char* buffer_;
};

class MasterSubWSFiber: public acl::fiber
{
    friend class MasterWSFiber;
public:
    MasterSubWSFiber(MasterWSFiber*, void*, int);
    ~MasterSubWSFiber();

protected:
    void run(void);

private:
    void* buffer_;
    int bufLen_;
    WSHandler handler_;
    NetPkgHeader stHeader_;
    MasterWSFiber* ws_fiber_;
};

#endif
