#ifndef _MASTER_ROUTE_H__
#define _MASTER_ROUTE_H__
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>

class MasterRoute
{
    typedef std::pair<std::string, int> server_host;
    typedef std::unordered_map<uint32_t, server_host> hash_addr;
    MasterRoute();
public:
    ~MasterRoute();
public:
    static MasterRoute* getInstance();
    void init();
    void dump();
    int add_route(const char*);

    const hash_addr& getSvrList() { return hash_route_; }
    const server_host& getDefault() { return default_route_; }
private:
    int parse_addr(const std::string&, server_host&);
private:
    hash_addr hash_route_;
    server_host default_route_;
};

#endif // !_MASTER_ROUTE_H__
