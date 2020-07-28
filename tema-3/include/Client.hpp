#pragma once

#include <nlohmann/json.hpp>

#include <netinet/in.h>

#include <cstdint>

#include <any>
#include <string>
#include <vector>
#include <unordered_map>

class Client {
    static std::string const& HOST_STR;
    static in_port_t const PORT;
    static std::string const& PORT_STR;

    static std::uint8_t* find(std::uint8_t*, std::uint8_t*,
                              std::string const&);

    static bool equal(std::uint8_t*, std::string const&);

    static void meta_add_start(std::ostringstream&,
                               std::string const&, std::string const&);

    void meta_add_hdrs(std::ostringstream&);
    void meta_add_content_hdrs(std::ostringstream&, std::string const&);

    int sockfd;
    std::vector<std::uint8_t> buf;
    std::unordered_map<std::string, std::any> resp;

    std::string cookie;
    std::string jwt;

public:
    Client();
    ~Client();

private:
    void connect();
    void close();

    bool send(std::string const&);
    bool recv();

public:
    void clear();

    std::pair<std::uint16_t, nlohmann::json>
    get(std::string const&);

    std::pair<std::uint16_t, nlohmann::json>
    post(std::string const&, nlohmann::json);

    std::pair<std::uint16_t, nlohmann::json>
    del(std::string const&);
};
