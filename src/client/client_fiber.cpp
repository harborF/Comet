#include "client_fiber.h"

ClientFiber::ClientFiber(ClientThread* t)
    : thread_(t)
{
    thread_->login(this);
}

ClientFiber::~ClientFiber(void)
{
    thread_->logout(this);
}

void ClientFiber::run(void)
{
    client_ = acl_vstream_connect(thread_->get_server_addr(), ACL_BLOCKING, 0, 0, 1024);
    if (NULL == client_) {
        acl_msg_error("fiber-%d: connect %s error %s",
            acl_fiber_self(), thread_->get_server_addr(), acl_last_serror());
        delete this;
        return;
    }

    acl::string strReq;
    std::srand((unsigned)time(NULL));
    strReq.format("{\"member_id\":%d,\"channel\":0,\"type\":1,\"sign\":\"123\",\"session\":\"testsession\"}",
        std::rand() % 2000000 + ACL_VSTREAM_SOCK(client_));
    if (this->sendMsg(ConstUtils::kCmdAuthReq, strReq.c_str()) == false) {
        delete this;
        return;
    }
    acl_msg_info("\tfiber-%ld-%d running", thread_->thread_id(), get_id());

    char  buf[256];
    int   ret = 0;
    NetPkgHeader stHeader;
    while (NULL != client_)
    {
        ret = acl_vstream_readn(client_, buf, sizeof(NetPkgHeader));
        if (ret == ACL_VSTREAM_EOF) {
            acl_msg_error("fiber-%d Failed to get: %d, fd: %d",
                get_id(), get_errno(), ACL_VSTREAM_SOCK(client_));
            break;
        }
        CommUtils::readNetHeader(buf, stHeader);
        const unsigned int uiDataLen = stHeader.uiPkgLen - stHeader.usHdrLen;
        if (uiDataLen > 256) {
            acl_msg_error("fiber-%d Failed to get: %d, fd: %d data len: %u",
                get_id(), get_errno(), ACL_VSTREAM_SOCK(client_), uiDataLen);
            break;
        }
        else if (uiDataLen) {
            ret = acl_vstream_readn(client_, buf, uiDataLen);
            if (ret == ACL_VSTREAM_EOF) {
                acl_msg_error("fiber-%d Failed to get: %d, fd: %d",
                    get_id(), get_errno(), ACL_VSTREAM_SOCK(client_));
                break;
            }
        }
        else {
            ret = 0;
        }
        buf[ret] = 0;
        this->handle_msg(stHeader, buf);

        acl_msg_info("fiber-%ld-%d cmd-%u: %s %d",
            thread_->thread_id(), get_id(),
            stHeader.uiCmdId, buf, uiDataLen);

    }
    acl_msg_info("\tfiber-%d close", get_id());

    acl_vstream_close(client_);

    delete this;
}

bool ClientFiber::sendMsg(const unsigned int uiCmdId, const char* msg)
{
    if (NULL == client_) {
        return false;
    }

    NetPkgHeader stHeader;
    stHeader.usHdrLen = sizeof(NetPkgHeader);
    stHeader.uiCmdId = uiCmdId;
    stHeader.uiPkgLen = stHeader.usHdrLen + strlen(msg);

    char  buf[32];
    CommUtils::writeNetHeader(buf, stHeader);

    if (acl_vstream_writen(client_, buf, sizeof(NetPkgHeader)) == ACL_VSTREAM_EOF) {
        acl_msg_error("write error, fd: %d", ACL_VSTREAM_SOCK(client_));
        if (acl_fiber_self() != this->get_id())
            acl_fiber_ready(this->get_fiber());
        return false;
    }

    if (strlen(msg) > 0 && acl_vstream_writen(client_, msg, strlen(msg)) == ACL_VSTREAM_EOF) {
        acl_msg_error("write error, fd: %d", ACL_VSTREAM_SOCK(client_));
        if (acl_fiber_self() != this->get_id())
            acl_fiber_ready(this->get_fiber());
        return false;
    }

    return true;
}

void ClientFiber::handle_msg(const NetPkgHeader& stHdr, const char* szBuf)
{
    if (stHdr.uiCmdId == ConstUtils::kCmdPushMsgReq) {
        Json::Value jsInput;
        Json::Reader reader;

        if (!reader.parse(szBuf, jsInput)) {
            acl_msg_error("parse error %s", szBuf);
            return;
        }

        Json::Value jsOuput;
        jsOuput["code"] = 0;
        jsOuput["data"]["type"] = 1;
        jsOuput["data"]["num"] = 1;
        jsOuput["data"]["packageid"] = jsInput["packageid"];

        Json::FastWriter writer;
        const std::string str = writer.write(jsOuput);

        this->sendMsg(ConstUtils::kCmdPushMsgAck, str.c_str());
    }
}

