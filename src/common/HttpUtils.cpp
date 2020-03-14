#include "HttpUtils.h"

#include <sstream>
#ifndef USE_ACL_HTTP
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#endif

namespace HttpUtils {

    std::string getHttp(const char* szHost, const acl::string& aclUrl, const std::string& strData)
    {
#ifdef USE_ACL_HTTP
        acl::http_request req(szHost);
        acl::http_header& hdr = req.request_header();
        hdr.set_host(szHost).set_url(aclUrl.c_str()).set_keep_alive(false);

        if (strData.empty()) {
            hdr.set_content_type("text/plain; charset=utf-8");
            if (req.request(NULL, 0) == false) {
				acl_msg_error("[%s]url:[%s%s]", __func__, szHost, aclUrl.c_str());
                return std::string();
            }
        }
        else {
            hdr.set_content_type("text/json;charset=utf-8");
            if (req.request(strData.c_str(), strData.length()) == false) {
				acl_msg_error("[%s]url:[%s%s] data:%s", __func__, szHost, aclUrl.c_str(), strData.c_str());
                return std::string();
            }
        }

        acl::string body;
        if (req.get_body(body, "utf-8") == false) {
			acl_msg_error("[%s]url:[%s%s] %s", __func__, szHost, aclUrl.c_str(), strData.c_str());
            return std::string();
        }
		acl_msg_info("[%s]url:[%s%s] %s %s", __func__, szHost, aclUrl.c_str(), strData.c_str(), body.c_str());

        return std::string(body.c_str());
#else
        std::string strUrl("http://");
        strUrl.append(szHost).append(aclUrl);
        try
        {
            curlpp::Easy request;
            request.setOpt(new curlpp::options::Url(strUrl));
            request.setOpt(new curlpp::options::Verbose(false));
            request.setOpt(new curlpp::options::NoSignal(true));
            request.setOpt(new curlpp::options::ForbidReuse(true));

            if (0 == strUrl.compare(0, 8, "https://")) {
                request.setOpt(new curlpp::options::SslVerifyPeer(false));
                request.setOpt(new curlpp::options::SslVerifyHost(false));
            }

            std::list<std::string> header;
            if (strData.empty()) {
                header.push_back("Content-Type: application/octet-stream");

                acl_msg_info("url:%s", strUrl.c_str());
            }
            else {
                header.push_back("Content-Type: text/json;charset=utf-8");

                request.setOpt(new curlpp::options::PostFields(strData.c_str()));
                request.setOpt(new curlpp::options::PostFieldSize(strData.length()));

                acl_msg_info("url:(%s) %s", strUrl.c_str(), strData.c_str());
            }

            request.setOpt(new curlpp::options::HttpHeader(header));

            std::ostringstream os;
            curlpp::options::WriteStream ws(&os);
            request.setOpt(ws);
            request.perform();
            return os.str();
        }
        catch (curlpp::LogicError & e) {
            acl_msg_error("%s:%s", __func__, e.what());
        }
        catch (curlpp::RuntimeError & e) {
            acl_msg_error("%s:%s", __func__, e.what());
        }
        catch (std::runtime_error &e) {
            acl_msg_error("%s:%s", __func__, e.what());
        }
        return std::string();
#endif
    }

    int getSession(const ST_Session& session, uint32_t& uiMemberId)
    {
        char szBuf[256];
        snprintf(szBuf, 256,
            "/session/getsession?session=%s&token=%s&channel=1&type=%s",
            session.session.c_str(), session.token.c_str(), session.type.c_str());
        const std::string strRet = getHttp(session.host.c_str(), szBuf);

        acl_msg_info("get session:%s %s", szBuf, strRet.c_str());
        if (strRet.empty()) {
            return PLY_INTERNAL_ERROR;
        }

        Json::Value jsSession;
        Json::Reader reader(Json::Features::strictMode());
        if (!reader.parse(strRet, jsSession) || PROTRULY_OK != jsSession["code"].asInt()) {
            return jsSession.isNull() ? PLY_INTERNAL_ERROR : jsSession["code"].asInt();
        }

        uiMemberId = jsSession["member_id"].asUInt();

        return 0;
    }

    int updateToken(const ST_Session& session, std::string& newToken)
    {
        char szBuf[256];
        snprintf(szBuf, 256,
            "/session/updatesession?session=%s&token=%s&channel=1&type=%s",
            session.session.c_str(), session.token.c_str(), session.type.c_str());
        const std::string strRet = getHttp(session.host.c_str(), szBuf);

        acl_msg_info("update token:%s %s", szBuf, strRet.c_str());
        if (strRet.empty()) {
            return PLY_INTERNAL_ERROR;
        }

        Json::Value jsSession;
        Json::Reader reader(Json::Features::strictMode());
        if (!reader.parse(strRet, jsSession) || PROTRULY_OK != jsSession["code"].asInt()) {
            return jsSession.isNull() ? PLY_INTERNAL_ERROR : jsSession["code"].asInt();
        }

        newToken = jsSession["token"].asString();

        return PROTRULY_OK;
    }

}//end namespace

