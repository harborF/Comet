#include "svr_job.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/processor/TMultiplexedProcessor.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;

using namespace  ::protruly;

int main(int argc, char **argv)
{
    acl::acl_cpp_init();
    acl::log::stdout_open(true);
    acl::log::open("/DISKB/log/thrift.log", "thrift");

    int port = 9090;
    boost::shared_ptr<StateSvrHandler> handler(new StateSvrHandler());

    boost::shared_ptr<TMultiplexedProcessor> processor(new TMultiplexedProcessor());
    processor->registerProcessor("StateSvr", boost::shared_ptr<TProcessor>( new StateSvrProcessor(handler)));
    
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(20);
    threadManager->threadFactory(boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory()));
    threadManager->start();

    TNonblockingServer server(processor, protocolFactory, port, threadManager);
    try {
        server.serve();
    }
    catch (TException& e) {
        printf("Server.serve() failed\n");
        exit(-1);
    }

    return 0;
}

