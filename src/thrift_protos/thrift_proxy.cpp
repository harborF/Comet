#include "thrift_proxy.h"
#include "server/master_route.h"

ThriftPool::ThriftPool():uiMaxConn_(5)
{
    rwlock_pool_ = acl_fiber_rwlock_create();
    this->reload();
}

ThriftPool::~ThriftPool()
{
    this->clear();
    acl_fiber_rwlock_free(rwlock_pool_);
}

void ThriftPool::clear()
{
    acl_fiber_rwlock_wlock(rwlock_pool_);
    for (auto it = vec_conn_.begin(); it != vec_conn_.end(); ++it) {
        acl_fiber_sem_free((*it)->sem_);
        acl_fiber_mutex_free((*it)->mutex_);
    }
    vec_conn_.clear();
    sock_pool_.clear();
    acl_fiber_rwlock_wunlock(rwlock_pool_);
}

void ThriftPool::add_config(const uint32_t uiCmdId, const std::string& strHost, const uint32_t uiPort)
{
    ST_Entry* ptr = nullptr;
    for (auto it = vec_conn_.begin(); it != vec_conn_.end(); ++it) {
        if ((*it)->strHost_.compare(strHost) == 0 && (*it)->uiPort_ == uiPort) {
            ptr = *it;
            break;
        }
    }

    if (ptr == nullptr) {
        ptr = new ST_Entry();
        ptr->sem_ = acl_fiber_sem_create(uiMaxConn_);
        ptr->mutex_ = acl_fiber_mutex_create();
        ptr->strHost_ = strHost;
        ptr->uiPort_ = uiPort;
        vec_conn_.push_back(ptr);
    }
    sock_pool_[uiCmdId] = ptr;
}

void ThriftPool::reload()
{
    acl_fiber_rwlock_wlock(rwlock_pool_);
    for (auto it = vec_conn_.begin(); it != vec_conn_.end(); ++it) {
        acl_fiber_sem_free((*it)->sem_);
        acl_fiber_mutex_free((*it)->mutex_);
        delete *it;
    }
    vec_conn_.clear();
    sock_pool_.clear();

    MasterRoute*const ptrRoute = MasterRoute::getInstance();
    for (auto it2 = ptrRoute->getSvrList().begin();
        it2 != ptrRoute->getSvrList().end();
        ++it2) {
        const std::pair<std::string, int>& serverHost = it2->second;
        this->add_config(it2->first, serverHost.first, serverHost.second);
    }
    this->add_config(0, ptrRoute->getDefault().first, ptrRoute->getDefault().second);

    acl_fiber_rwlock_wunlock(rwlock_pool_);
}

ThriftPool::ST_Entry* ThriftPool::getCoonPool(const uint32_t uiCmdId)
{
    hash_socket::iterator itF = sock_pool_.find(uiCmdId);
    if (itF != sock_pool_.end()) {
        return itF->second;
    }

    itF = sock_pool_.find(0);
    if (itF != sock_pool_.end()) {
        return itF->second;
    }
    return nullptr;
}

boost::shared_ptr<TSocket> ThriftPool::getSocket(const uint32_t uiCmdId)
{
    acl_fiber_rwlock_rlock(rwlock_pool_);
    ST_Entry*const ptrPool = getCoonPool(uiCmdId);
    if (ptrPool == nullptr) {
        acl_msg_error("[%s]>>thrift command: %u\n", __func__, uiCmdId);
        acl_fiber_rwlock_runlock(rwlock_pool_);
        return boost::shared_ptr<TSocket>();
    }

    acl_fiber_sem_wait(ptrPool->sem_);
    boost::shared_ptr<TSocket> ptr;
    acl_fiber_mutex_lock(ptrPool->mutex_);
    while (ptrPool->q_sock_.size()) {
        ptr = ptrPool->q_sock_.front();
        ptrPool->q_sock_.pop();
        if (ptr->isOpen()) {
            acl_fiber_mutex_unlock(ptrPool->mutex_);
            acl_fiber_rwlock_runlock(rwlock_pool_);
            return ptr;
        }
    }
    acl_fiber_mutex_unlock(ptrPool->mutex_);
    acl_fiber_rwlock_runlock(rwlock_pool_);

    ptr.reset(new TSocket(ptrPool->strHost_, ptrPool->uiPort_));
    ptr->setKeepAlive(true);

    return ptr;
}

void ThriftPool::releaseSocket(const uint32_t uiCmdId, const boost::shared_ptr<TSocket>& ptr)
{
    acl_fiber_rwlock_rlock(rwlock_pool_);
    ST_Entry*const ptrPool = getCoonPool(uiCmdId);
    if (ptrPool == nullptr) {
        acl_msg_error("[%s]>>thrift command: %u\n", __func__, uiCmdId);
        acl_fiber_rwlock_runlock(rwlock_pool_);
        return;
    }

    acl_fiber_sem_post(ptrPool->sem_);
    acl_fiber_mutex_lock(ptrPool->mutex_);
    if (ptr == nullptr || !ptr->isOpen()) {
        acl_fiber_mutex_unlock(ptrPool->mutex_);
        acl_fiber_rwlock_runlock(rwlock_pool_);
        return;
    }

    ptrPool->q_sock_.push(ptr);
    while (sock_pool_.size() > uiMaxConn_) {
        ptrPool->q_sock_.pop();
    }
    acl_fiber_mutex_unlock(ptrPool->mutex_);
    acl_fiber_rwlock_runlock(rwlock_pool_);
}

//////////////////////////////////////////////////////////////////////
ThriftProxy::ThriftProxy(const uint32_t id, ThriftPool*const pool) :
    uiCmdId_(id), pool_(pool)
{
    socket_ = pool_->getSocket(uiCmdId_);
}

ThriftProxy::~ThriftProxy()
{
    pool_->releaseSocket(uiCmdId_, socket_);
}

boost::shared_ptr<TProtocol> ThriftProxy::getProtocl()
{
    boost::shared_ptr<TTransport> transport(new TFramedTransport(socket_));
    try {
        transport->open();
    }
    catch (TException& e) {
        acl_msg_error("[%s]>>thrift open: %s\n", __func__, e.what());
    }

    return boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
}

boost::shared_ptr<TProtocol> ThriftProxy::getMultiProtocl(const std::string& serviceName)
{
    return boost::shared_ptr<TProtocol>(new TMultiplexedProtocol(getProtocl(), serviceName));
}
