#include "master_thread.h"
#include "master_fiber.h"
#include <signal.h>

#define	STACK_SIZE (32*1024)
extern MasterConfigVar g_CfgVar;

static void usage(const char *procname) {
    printf("usage: %s -h [help]\r\n"
        " -f config file path\r\n", procname);
}

std::vector<acl::thread*> g_threads;
std::vector<ACL_MBOX*> g_mbox;

int push_msg(const uint32_t uiTid, const MPostMsg& cMsg)
{
    if (uiTid) {
        if (uiTid > g_CfgVar.uiMaxThreads) {
            acl_msg_error("[%s]%u", __func__, uiTid);
            return -1;
        }

        MPostMsg *ptrMsg = new MPostMsg(cMsg);
        if (acl_mbox_send(g_mbox[uiTid - 1], ptrMsg) < 0) {
            delete ptrMsg;
        }
    }
    else if (0 == uiTid) {
        for (uint32_t i = 0; i < g_CfgVar.uiMaxThreads; ++i) {

            MPostMsg *ptrMsg = new MPostMsg(cMsg);
            if (acl_mbox_send(g_mbox[i], ptrMsg) < 0) {
                delete ptrMsg;
            }
        }
    }

    return 0;
}

static void fiber_accept(ACL_FIBER *, void *ctx)
{
    const char sockType = (char)(long)ctx;
    char* sockAddr = NULL;
    switch (sockType)
    {
    case 'c':
        sockAddr = g_CfgVar.tcp_port;
        break;
    case 'h':
        sockAddr = g_CfgVar.http_port;
        break;
    case 'w':
        sockAddr = g_CfgVar.ws_port;
        break;
    default:
        acl_msg_fatal("socket type %c error\r\n", sockType);
        return exit(1);
    }

    acl::server_socket server;
    if (server.open(sockAddr) == false) {
        acl_msg_fatal("open %s error\r\n", sockAddr);
        return exit(1);
    }
    printf(">>listen %s ok\r\n", sockAddr);

    while (true) {
        acl::socket_stream* cstream = server.accept();
        if (cstream == NULL) {
            acl_msg_error("accept failed: %s", acl::last_serror());
            break;
        }

        cstream->set_rw_timeout(0);
		cstream->set_tcp_non_blocking(false);
        unsigned int uiIdx = cstream->sock_handle() % g_CfgVar.uiMaxThreads;
        MPostMsg *ptrMsg = new MPostMsg(sockType, cstream);
        if (acl_mbox_send(g_mbox[uiIdx], ptrMsg) < 0) {
            delete cstream;
            delete ptrMsg;
        }
    }

    exit(0);
}

static void fiber_sleep_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
    while (1) {
        acl_fiber_sleep(g_CfgVar.wakeup_time);

        checkRollDailyLog();

        //acl_msg_info("wakeup, cost %d seconds", g_CfgVar.wakeup_time);

        for (unsigned int i = 0; i < g_CfgVar.uiMaxThreads; ++i) {
            MPostMsg *ptrMsg = new MPostMsg(CMD_TIMEOUT_ID, NULL);
            acl_mbox_send(g_mbox[i], ptrMsg);
        }
    }
}

void start_work_thread()
{
    for (uint32_t i = g_threads.size(); i < g_CfgVar.uiMaxThreads; ++i) {
        ACL_MBOX* mbox = acl_mbox_create();
        g_mbox.push_back(mbox);

        MasterThread* thread = new MasterThread(i + 1, mbox);
        thread->set_detachable(false);
        //thread->set_stacksize(STACK_SIZE * (g_CfgVar.uiMaxFibers + 6400));
        g_threads.push_back(thread);
        thread->start();
    }

    acl_msg_info(">>start thread num-------%lu", g_threads.size());
}

#ifdef SIGUSR1
void signal_handler(int signo)
{
    if (signo == SIGUSR1) {
        printf("received SIGUSR1\n");
        loadConfigAndOpenLog(g_CfgVar.configPath);

        start_work_thread();
    }
    else if (signo == SIGUSR2) {
        printf("received SIGUSR2\n");
        for (unsigned int i = 0; i < g_CfgVar.uiMaxThreads; ++i) {
            MPostMsg *ptrMsg = new MPostMsg(CMD_DUMP_ID, NULL);
            acl_mbox_send(g_mbox[i], ptrMsg);
        }
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    int ch;
    g_CfgVar.configPath.format("%s.cf", argv[0]);

    while ((ch = getopt(argc, argv, "h:f:")) > 0)
    {
        switch (ch) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'f':
            g_CfgVar.configPath = optarg;
            break;
        default:
            break;
        }
    }

    /*
    * Don't die when a process goes away unexpectedly.
    */
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    /*
    * Don't die for frivolous reasons.
    */
#ifdef SIGXFSZ
    signal(SIGXFSZ, SIG_IGN);
#endif
    acl::acl_cpp_init();
    acl::log::stdout_open(false);
#ifdef SIGUSR1
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
#endif    

    loadConfigAndOpenLog(g_CfgVar.configPath);
    acl_assert(g_CfgVar.uiFiberStackSize >= 32 * 1024);
    acl_assert(g_CfgVar.uiMaxReadBuffer >= 256);

    start_work_thread();

    if (0 == strcmp("tcp", g_CfgVar.master_type)) {
        acl_msg_info(">>accept tcp: %s\t\taccept http: %s\r\n", g_CfgVar.tcp_port, g_CfgVar.http_port);
        acl_fiber_create(fiber_accept, (char*)'c', STACK_SIZE);
        acl_fiber_create(fiber_accept, (char*)'h', STACK_SIZE);
    }
    else if (0 == strcmp("ws", g_CfgVar.master_type)) {
        acl_msg_info(">>accept ws: %s\t\taccept http: %s\r\n", g_CfgVar.ws_port, g_CfgVar.http_port);
        acl_fiber_create(fiber_accept, (char*)'w', STACK_SIZE);
        acl_fiber_create(fiber_accept, (char*)'h', STACK_SIZE);
    }
    else {
        acl_msg_info(">>accept tcp: %s\t\taccept http: %s\r\n", g_CfgVar.tcp_port, g_CfgVar.http_port);

        acl_fiber_create(fiber_accept, (char*)'c', STACK_SIZE);
        acl_fiber_create(fiber_accept, (char*)'h', STACK_SIZE);
    }
    acl_fiber_create(fiber_sleep_main, NULL, STACK_SIZE);

    init_curlcpp_fun();

    acl_fiber_schedule();

    for (uint32_t i = 0; i < g_threads.size(); ++i) {
        g_threads[i]->wait(NULL);
        delete g_threads[i];        
    }
    acl::log::close();

    return 0;
}
