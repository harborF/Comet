#include "master_http.h"
#include "master_config.h"

MasterHttpServlet::MasterHttpServlet(MasterHttpFiber* f, acl::socket_stream* stream)
    : fiber_(f), HttpServlet(stream)
{
}

MasterHttpServlet::~MasterHttpServlet(void)
{
}

// override
bool MasterHttpServlet::doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
{
    return doPost(req, res);
}

// override
bool MasterHttpServlet::doPost(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
{
    HttpHandler handler = getHttpHandler(req.getPathInfo());
    if (NULL != handler) {
        return handler(this, req, res) == 0;
    }
    acl_msg_error("%s", req.getRequestUri());

    // ·¢ËÍ http ÏìÓ¦Ìå
    static int __i = 0;
    return res.format("hello-%d\r\n", __i++) && res.write(NULL, 0) && req.isKeepAlive();
}

bool MasterHttpServlet::doError(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
{
    acl_msg_error("http request error");

    return false;
}

MasterThread* MasterHttpServlet::getThread() {
    return fiber_->getThread(); 
}

bool MasterHttpServlet::responseCode(const int nCode, acl::HttpServletResponse& res)
{
    acl::string str;
    str.format("{\"code\":%d}", nCode);
    res.setContentLength(str.length());
    return res.write(str.c_str(), str.length()) && res.write(NULL, 0);
}

bool MasterHttpServlet::responseJson(Json::Value& jsValue, acl::HttpServletResponse& res)
{
    static Json::FastWriter writer;
    const std::string str = writer.write(jsValue);
    res.setContentLength(str.length());
    return res.write(str.c_str(), str.length()) && res.write(NULL, 0);
}

MasterHttpFiber::MasterHttpFiber(MasterThread* t, acl::socket_stream* c)
    :client_(c), thread_(t)
{

}

MasterHttpFiber::~MasterHttpFiber(void)
{

}

void MasterHttpFiber::run(void)
{
    acl_msg_info("[%s]http-fiber-%ld-%d running", __func__, thread_->thread_id(), get_id());

    MasterHttpServlet servlet(this, client_);
    servlet.setLocalCharset("utf-8");

    while (true) {
        if (servlet.doRun() == false)
            break;
    }

    acl_msg_info("[%s]fiber-%ld-%d close http", __func__, thread_->thread_id(), get_id());

    client_->close();
    delete client_;
    delete this;
}
