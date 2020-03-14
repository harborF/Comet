#include "client_thread.h"
#include "client_fiber.h"

#define	STACK_SIZE (16*1024)

static void usage(const char *procname) {
    printf("usage: %s -h [help]\r\n"
        " -s server_addr\r\n"
        " -f fibers count\r\n"
        " -m threads_count\r\n", procname);
}

unsigned int g_MaxFibers = 1000;
unsigned int g_MaxThread = 2;
std::vector<acl::thread*> g_threads;
std::vector<ACL_MBOX*> g_mbox;

int push_msg(const uint32_t uiTid, const MPostMsg& cMsg)
{
    for (unsigned int i = 0; i < g_MaxThread; ++i) {
        MPostMsg *ptrMsg = new MPostMsg(cMsg);
        acl_mbox_send(g_mbox[uiTid - 1], ptrMsg);
    }
}

static void free_msg(void *ctx){
    MPostMsg *ptr = (MPostMsg *) ctx;
    if (ptr->ptrParam != NULL) {
        acl::socket_stream *client = (acl::socket_stream *)ptr->ptrParam;
        delete client;
    }
    delete ptr;
}

static void fiber_sleep_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
#define MAX_SPAN 4
    while (1) {
        for (unsigned int i = 0; i < g_MaxThread; ++i) {
            MPostMsg *ptrMsg = new MPostMsg('t', NULL);
            acl_mbox_send(g_mbox[i], ptrMsg);
        }  
        
        static int iCount = MAX_SPAN;
        if (iCount == MAX_SPAN) {
            for (unsigned int i = 0; i < g_MaxThread; ++i) {
                MPostMsg *ptrMsg = new MPostMsg('c', NULL);
                acl_mbox_send(g_mbox[i], ptrMsg);
            }
        }
        if (--iCount == 0)iCount = MAX_SPAN;

        acl_fiber_sleep(30);
    }
}

int main(int argc, char *argv[])
{
    int ch;
    acl::string server_addr(":8888");

    while ((ch = getopt(argc, argv, "hs:l:f:m:")) > 0)
    {
        switch (ch) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 's':
            server_addr = optarg;
            break;
        case 'f':
            g_MaxFibers = atoi(optarg);
            break;
        case 'm':
            g_MaxThread = atoi(optarg);
            break;
        default:
            break;
        }
    }

    acl::acl_cpp_init();
    acl::log::stdout_open(false);
    acl::log::open("/DISKB/client.log", "client");

    for (unsigned int i = 0; i < g_MaxThread; i++) {
        ACL_MBOX* mbox = acl_mbox_create();
        g_mbox.push_back(mbox);

        ClientThread* thread = new ClientThread(g_MaxFibers, STACK_SIZE, mbox, server_addr);
        thread->set_detachable(false);
        //thread->set_stacksize(STACK_SIZE * (g_MaxFibers/2 + 6400));
        g_threads.push_back(thread);
        thread->start();
    }

    acl_fiber_create(fiber_sleep_main, NULL, STACK_SIZE);

    acl_fiber_schedule();

    for (unsigned int i = 0; i < g_MaxThread; ++i) {
        g_threads[i]->wait(NULL);
        delete g_threads[i];

        acl_mbox_free(g_mbox[i], free_msg);
    }

    return 0;
}
