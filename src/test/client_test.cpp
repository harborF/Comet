#include "fiber/lib_fiber.h"
#include "fiber/lib_fiber.hpp"
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "common/CmdDefine.h"
#include "common/CommUtils.h"
#include "json/json.h"

ACL_VSTREAM* g_client_ = NULL;
const char* g_svr_addr = "192.168.19.34:60000";
#define	STACK_SIZE (16*1024)
unsigned int g_uiMemberId = 60001290;

static std::string getLogTime() {
    time_t t = time(NULL);
    struct tm* ptm = localtime(&t);

    char szBuf[32];
    strftime(szBuf, 32, "%Y-%m-%d %H:%M:%S", ptm);
    return std::string(szBuf);
}

bool sendMsg(const unsigned int uiCmdId, const char* msg)
{
    if (NULL == g_client_) {
        return false;
    }

    NetPkgHeader stHeader;
    stHeader.usHdrLen = sizeof(NetPkgHeader);
    stHeader.uiCmdId = uiCmdId;
    stHeader.uiPkgLen = stHeader.usHdrLen + strlen(msg);

    char buf[32];
    CommUtils::writeNetHeader(buf, stHeader);

    if (acl_vstream_writen(g_client_, buf, sizeof(NetPkgHeader)) == ACL_VSTREAM_EOF) {
        acl_msg_error("%s write error, fd: %d", getLogTime().c_str(), ACL_VSTREAM_SOCK(g_client_));
        return false;
    }

    if (strlen(msg) > 0 && acl_vstream_writen(g_client_, msg, strlen(msg)) == ACL_VSTREAM_EOF) {
        acl_msg_error("%s write error, fd: %d", getLogTime().c_str(), ACL_VSTREAM_SOCK(g_client_));
        return false;
    }

    return true;
}

void handle_msg(const NetPkgHeader& stHdr, const char* szBuf)
{
    if (stHdr.uiCmdId == ConstUtils::kCmdPushMsgReq) {
        Json::Value jsInput;
        Json::Reader reader;

        if (!reader.parse(szBuf, jsInput)) {
            acl_msg_error("%s parse error %s", getLogTime().c_str(), szBuf);
            return;
        }

        Json::Value jsOuput;
        jsOuput["code"] = 0;
        jsOuput["data"]["type"] = 1;
        jsOuput["data"]["num"] = 1;
        jsOuput["data"]["packageid"] = jsInput["packageid"];

        Json::FastWriter writer;
        const std::string str = writer.write(jsOuput);

        sendMsg(ConstUtils::kCmdPushMsgAck, str.c_str());
    }
}

static void fiber_callback(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
    g_client_ = acl_vstream_connect(g_svr_addr, ACL_BLOCKING, 0, 0, 1024);
    if (NULL == g_client_) {
        acl_msg_error("%s fiber-%d: connect %s error %s", getLogTime().c_str(),
            acl_fiber_self(), g_svr_addr, acl_last_serror());
        return;
    }

    acl::string strReq;
    std::srand((unsigned)time(NULL));
    strReq.format("{\"member_id\":%u,\"channel\":0,\"type\":1,\"sign\":\"123\",\"session\":\"testsession\"}",
        g_uiMemberId);
    if (sendMsg(ConstUtils::kCmdAuthReq, strReq.c_str()) == false) {
        return;
    }
    acl_msg_info("%s\tfiber-%d: running", getLogTime().c_str(), acl_fiber_self());

    char  buf[256];
    int   ret = 0;
    NetPkgHeader stHeader;
    while (NULL != g_client_)
    {
        ret = acl_vstream_readn(g_client_, buf, sizeof(NetPkgHeader));
        if (ret == ACL_VSTREAM_EOF) {
            acl_msg_error("%s Failed to fd: %d", getLogTime().c_str(), ACL_VSTREAM_SOCK(g_client_));
            break;
        }
        CommUtils::readNetHeader(buf, stHeader);
        const unsigned int uiDataLen = stHeader.uiPkgLen - stHeader.usHdrLen;
        if (uiDataLen > 256) {
            acl_msg_error("%s Failed to fd: %d data len: %u", getLogTime().c_str(), ACL_VSTREAM_SOCK(g_client_), uiDataLen);
            break;
        }
        else if (uiDataLen) {
            ret = acl_vstream_readn(g_client_, buf, uiDataLen);
            if (ret == ACL_VSTREAM_EOF) {
                acl_msg_error("%s Failed to fd: %d", getLogTime().c_str(), ACL_VSTREAM_SOCK(g_client_));
                break;
            }
        }
        else {
            ret = 0;
        }
        buf[ret] = 0;
        handle_msg(stHeader, buf);

        acl_msg_info("%s cmd-%u: len-%d buf-%s", getLogTime().c_str(), stHeader.uiCmdId, uiDataLen, buf);
    }
    acl_msg_info("%s\tfiber-%d: connect close", getLogTime().c_str(), acl_fiber_self());

    acl_vstream_close(g_client_);
}

int main(int argc, char **argv)
{
    acl::acl_cpp_init();
    acl::log::stdout_open(true);

    acl_fiber_create(fiber_callback, NULL, STACK_SIZE);
    acl_fiber_schedule();

    return 0;
}
