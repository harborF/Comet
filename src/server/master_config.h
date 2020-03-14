#ifndef _MASTER_CONFIG_H__
#define _MASTER_CONFIG_H__
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "common/CmdDefine.h"
#include "common/CommUtils.h"
#include "common/ErrorCode.h"
#include "common/HttpUtils.h"

#define USE_SUB_FIBER 0

struct MasterConfigVar {
    acl::string configPath;

    int  debug_enable;
    int  keep_alive;
    int  run_loop;

    int  io_timeout;
    int  wakeup_time;

    char *master_type;/*tcp ws*/    
    char *tcp_port;/*tcp port*/
    char *ws_port;/*web socket*/
    char *http_port;/**/
    char *http_host;/*show outside*/

    /**/
    char *login_host_port;
    char *redis_host_port;
    char *msg_host_port;
    char *thrift_svr_host;
    int thrift_svr_port;

    /*log*/
    char *log_path;
    char *master_name;

    /*thread*/
    int uiMaxThreads;
    int uiMaxFibers;
    int uiFiberStackSize;

    /*fiber*/
    int uiConnTimeout;
    int uiMaxReadBuffer;
};

void checkRollDailyLog(const bool bReOpen = false);
void loadConfigAndOpenLog(const char *pathname);

class MasterTcpFiber;
typedef int(*OprHandler)(const NetPkgHeader&, MasterTcpFiber*, const void*, const unsigned int);
class MasterHttpServlet;
typedef bool(*HttpHandler)(MasterHttpServlet*, acl::HttpServletRequest& req, acl::HttpServletResponse& res);
class MasterWSFiber;
typedef int(*WSHandler)(const NetPkgHeader&, MasterWSFiber*, const void*, const unsigned int);

OprHandler getHandler(const unsigned int);
HttpHandler getHttpHandler(const char*);
WSHandler getWSHandler(const unsigned int);

#endif // !MASTER_CONFIG_H__
