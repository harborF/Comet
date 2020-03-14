#include "master_config.h"
#include "master_fiber.h"
#include "master_route.h"

MasterConfigVar g_CfgVar;

static ACL_CFG_STR_TABLE var_conf_str_tab[] = {
    { "master_type", "all", &(g_CfgVar.master_type) },
    { "master_tcp_port", ":8000", &(g_CfgVar.tcp_port) },
    { "master_ws_port", ":8002", &(g_CfgVar.ws_port) },
    { "master_http_port", ":8004", &(g_CfgVar.http_port) },
    { "master_http_host", "127.0.0.1", &(g_CfgVar.http_host) },

    { "login_host_port", "127.0.0.1:8006", &(g_CfgVar.login_host_port) },
    { "redis_host_port", "127.0.0.1:6379", &(g_CfgVar.redis_host_port) },
    { "msg_host_port", "127.0.0.1:8008", &(g_CfgVar.msg_host_port) },
    { "thrift_svr_host", "127.0.0.1", &(g_CfgVar.thrift_svr_host) },

    { "master_log", "/tmp/master.log", &(g_CfgVar.log_path) },
    { "master_name", "unkown", &(g_CfgVar.master_name) },
    { 0, 0, 0 }
};

static ACL_CFG_BOOL_TABLE var_conf_bool_tab[] = {
    { "debug_enable", 1, &(g_CfgVar.debug_enable) },
    { "keep_alive", 1, &(g_CfgVar.keep_alive) },
    { "loop_read", 1, &(g_CfgVar.run_loop) },
    { 0, 0, 0 }
};

static ACL_CFG_INT_TABLE var_conf_int_tab[] = {
    { "io_timeout", 0, &(g_CfgVar.io_timeout), 0, 120 },
    { "master_wakeup", 20, &(g_CfgVar.wakeup_time), 20, 120 },

    { "master_max_threads", 2, &(g_CfgVar.uiMaxThreads), 2, 100 },
    { "thread_max_fibers", 100, &(g_CfgVar.uiMaxFibers), 100, 30000 },
    { "fiber_stacksize", 32 * 1024, &(g_CfgVar.uiFiberStackSize), 32 * 1024, 128 * 1024 },

    { "read_buffer_size", 256, &(g_CfgVar.uiMaxReadBuffer), 256, 2 * 1024 },
    { "conn_timeout", 100, &(g_CfgVar.uiConnTimeout), 100, 5 * 60 },

    { "thrift_svr_port", 9090, &(g_CfgVar.thrift_svr_port), 3000, 65535 },

    { 0, 0 , 0 , 0, 0 }
};

std::string getYMD() {
    time_t now;
    time(&now);
    struct tm *tmNow = localtime(&now);

    char szBuf[32];
    strftime(szBuf, 32, "%Y%m%d", tmNow);
    return std::string(szBuf);
}

void checkRollDailyLog(const bool bReOpen)
{
    static std::string strDaily;
    const std::string strNewDaily = getYMD();

    if (0 != strDaily.compare(strNewDaily) || bReOpen) {
        strDaily = strNewDaily;
        
        std::stringstream ss;
        ss << g_CfgVar.log_path << "/" << g_CfgVar.master_name
            << strNewDaily << ".log";

        acl::log::close();
        acl::log::open(ss.str().c_str(), g_CfgVar.master_name);
    }
}

void loadConfigAndOpenLog(const char *pathname)
{
    ACL_XINETD_CFG_PARSER * cfg = acl_xinetd_cfg_load(pathname);
    if (NULL == cfg) {
        acl_msg_error("[%s]read config file failed", __func__);
        return;
    }

    // 设置配置参数表
    acl_xinetd_params_int_table(cfg, var_conf_int_tab);
    acl_xinetd_params_str_table(cfg, var_conf_str_tab);
    acl_xinetd_params_bool_table(cfg, var_conf_bool_tab);
    
    checkRollDailyLog(true);

    uint32_t i = 0;
    acl_msg_info("---load config--------------------------");
    for (i = 0; var_conf_str_tab[i].name != 0; ++i)
        acl_msg_info(">>%s-----%s", var_conf_str_tab[i].name, *(var_conf_str_tab[i].target));
    for (i = 0; var_conf_bool_tab[i].name != 0; ++i)
        acl_msg_info(">>%s-----%d", var_conf_bool_tab[i].name, *(var_conf_bool_tab[i].target));
    for (i = 0; var_conf_int_tab[i].name != 0; ++i)
        acl_msg_info(">>%s-----%d", var_conf_int_tab[i].name, *(var_conf_int_tab[i].target));

    MasterRoute::getInstance()->init();
    const ACL_ARRAY* ptrArr = acl_xinetd_cfg_get_ex(cfg, "svr_route");
    if (ptrArr) {
        for (int i = 0; i < acl_array_size(ptrArr); ++i) {
            acl_msg_info(">>svr_route: %s", (const char *) acl_array_index(ptrArr, i));
            MasterRoute::getInstance()->add_route((const char *) acl_array_index(ptrArr, i));
        }
    }//end if
    MasterRoute::getInstance()->dump();

    acl_xinetd_cfg_free(cfg);

    acl_msg_info("---------------------------------------");
}

