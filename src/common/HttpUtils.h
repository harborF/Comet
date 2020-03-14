#ifndef _HTTP_UTILS_H__
#define _HTTP_UTILS_H__
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "json/json.h"

#define PROTRULY_OK 0
#define PLY_INTERNAL_ERROR -101

#define ENUM_TOKEN_STORE_READ "r"
#define ENUM_TOKEN_STORE_WRITE "w"
#define ENUM_TOKEN_AVATOR_WRITE "u"

struct ST_Session {
    std::string session;
    std::string token;
    std::string type;
    std::string host;
};
const std::string c_NULLSTR;

#define USE_ACL_HTTP 1

#ifdef USE_ACL_HTTP
#define init_curlcpp_fun() void();
#else
#include <curlpp/cURLpp.hpp>
#define init_curlcpp_fun() curlpp::Cleanup cleaner;
#endif

namespace HttpUtils {
    std::string getHttp(const char* szHost, const acl::string& aclUrl, const std::string& strData = c_NULLSTR);

    int getSession(const ST_Session& session, uint32_t& uiMemberId);
    int updateToken(const ST_Session& session, std::string& newToken);
}

#endif // !_HTTP_UTILS_H__
