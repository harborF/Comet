#include "master_route.h"
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

MasterRoute::MasterRoute()
{
}

MasterRoute::~MasterRoute()
{
}

MasterRoute* MasterRoute::getInstance()
{
    static MasterRoute s_Instance;
    return &s_Instance;
}

void MasterRoute::init()
{
    this->hash_route_.clear();
}

std::string dump_addr(const std::pair<std::string, int>& svrHost) {
    std::stringstream ss;
    ss << svrHost.first.c_str() << ":" << svrHost.second << "\t";

    return ss.str();
}

void MasterRoute::dump()
{
    for (hash_addr::const_iterator it = hash_route_.begin(); it != hash_route_.end(); ++it) {
        acl_msg_info(">>dump route: %u %s", it->first, dump_addr(it->second).c_str());
    }
}

int MasterRoute::add_route(const char* szRoute)
{
    const std::string strRoute(szRoute);
    size_t uiPos_1 = strRoute.find_first_of(":");
    if (uiPos_1 == std::string::npos) {
        return -1;
    }

    server_host vecAddr;
    if (parse_addr(strRoute, vecAddr)) {
        return -2;
    }

    const std::string strCmdId = strRoute.substr(0, uiPos_1);
    if (0 == strCmdId.compare("*")) {
        default_route_ = vecAddr;
        return 0;
    }

    uiPos_1 = strCmdId.find("-");
    uint32_t uiIdBegin = 0, uiIdEnd = 0;
    if (uiPos_1 == std::string::npos) {
        uiIdBegin = uiIdEnd = (uint32_t)strtoul(strCmdId.c_str(), NULL, 0);
    }
    else {
        const std::string strBegin = strCmdId.substr(0, uiPos_1);
        const std::string strEnd = strCmdId.substr(uiPos_1 + 1);
        uiIdBegin = (uint32_t)strtoul(strBegin.c_str(), NULL, 0);
        uiIdEnd = (uint32_t)strtoul(strEnd.c_str(), NULL, 0);
    }//end if

    for (uint32_t i = uiIdBegin; i <= uiIdEnd; ++i) {
        hash_route_[i] = vecAddr;
    }

    return 0;
}

std::vector<std::string> splitEx(const std::string& src, const std::string& sep_str)
{
    std::vector<std::string> vecStrs;
    const size_t sep_Len = sep_str.size();
    size_t lastPos = 0, index = std::string::npos;
    while (std::string::npos != (index = src.find(sep_str, lastPos))) {
        vecStrs.push_back(src.substr(lastPos, index - lastPos));
        lastPos = index + sep_Len;
    }

    std::string lastString = src.substr(lastPos);
    if (!lastString.empty())
        vecStrs.push_back(lastString);

    return vecStrs;
}

int MasterRoute::parse_addr(const std::string& strRoute, server_host& svrAddr)
{
    size_t uiPos_1 = strRoute.find_first_of("[");
    size_t uiPos_2 = strRoute.find_last_of("]");
    if (uiPos_1 == std::string::npos || uiPos_2 == std::string::npos || uiPos_1 > uiPos_2) {
        return -1;
    }
    const std::string strAddr = strRoute.substr(uiPos_1 + 1, uiPos_2 - uiPos_1 - 1);

    std::vector<std::string> vecHostPort = splitEx(strAddr, ",");
    if (0 == vecHostPort.size()) {
        return -2;
    }

    for (std::vector<std::string>::iterator it = vecHostPort.begin(); it != vecHostPort.end(); ++it) {
        std::vector<std::string> vec = splitEx(*it, ":");
        if (vec.size() != 2)
            return -3;
        svrAddr = { vec[0], (uint32_t)strtoul(vec[1].c_str(), NULL, 0) };
    }

    return 0;
}

