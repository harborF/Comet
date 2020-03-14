#ifndef _OPR_HANDLER_H__
#define _OPR_HANDLER_H__
#include "server/master_config.h"
#include "server/master_http.h"
#include "server/conn_mgr.h"

//tcp
int login_handler(const NetPkgHeader&, MasterTcpFiber*, const void*, const unsigned int);
int heart_handler(const NetPkgHeader&, MasterTcpFiber*, const void*, const unsigned int);

int push_ack_handler(const NetPkgHeader&, MasterTcpFiber*, const void*, const unsigned int);

//http
bool push_handler(MasterHttpServlet* servlet, acl::HttpServletRequest& req, acl::HttpServletResponse& res);
bool logout_handler(MasterHttpServlet* servlet, acl::HttpServletRequest& req, acl::HttpServletResponse& res);

//web socket
int login_handler(const NetPkgHeader&, MasterWSFiber*, const void*, const unsigned int);
int heart_handler(const NetPkgHeader&, MasterWSFiber*, const void*, const unsigned int);

int push_ack_handler(const NetPkgHeader&, MasterWSFiber*, const void*, const unsigned int);


#endif
