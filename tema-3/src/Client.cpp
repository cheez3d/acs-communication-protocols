#include "Client.hpp"

#include "utils.hpp"

#include <nlohmann/json.hpp>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>

#include <any>
#include <algorithm>
#include <sstream>
#include <vector>

namespace {
    std::string const& PROTO_VER = "HTTP/1.1";

    std::string const& META_TERM = "\r\n";
    std::string const& META_END = META_TERM+META_TERM;
    std::string const& META_MIME = "application/json";

    std::string const& HDR_SEP = ":";

    std::string const& HDR_COOKIE_SEP = ";";
}

std::string const& Client::HOST_STR = "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
in_port_t const Client::PORT = 8080;
std::string const& Client::PORT_STR = std::to_string(Client::PORT);

std::uint8_t* Client::find(std::uint8_t* begin,
                           std::uint8_t* end,
                           std::string const& s)
{
    return std::search(begin, end, s.begin(), s.end());
}

bool Client::equal(std::uint8_t* begin, std::string const& s) {
    return std::equal(s.begin(), s.end(), begin);
}

void Client::meta_add_start(std::ostringstream& oss,
                            std::string const& method,
                            std::string const& target)
{
    oss << method << " " << target << " " << PROTO_VER << META_TERM;
}

void Client::meta_add_hdrs(std::ostringstream& oss) {
    oss << "Host"           << HDR_SEP << " " << HOST_STR  << META_TERM;
//	oss << "Connection"     << HDR_SEP << " " << "close"   << META_TERM;
    oss << "Accept"         << HDR_SEP << " " << META_MIME << META_TERM;

    if (!cookie.empty()) {
        oss << "Cookie" << HDR_SEP << " " << cookie << META_TERM;
    }

    if (!jwt.empty()) {
        oss << "Authorization" << HDR_SEP << " "
            << "Bearer" << " " << jwt << META_TERM;
    }
}

void Client::meta_add_content_hdrs(std::ostringstream& oss,
                                   std::string const& content)
{
    oss << "Content-Type"   << HDR_SEP << " " << META_MIME        << META_TERM;
    oss << "Content-Length" << HDR_SEP << " " << content.length() << META_TERM;
}

Client::Client() {
    sockfd = -1;
}

Client::~Client() {
    close();
}

void Client::connect() {
    int ret;

    close();

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd == -1, "socket");

    struct addrinfo addrinfo_hints = {
        ai_flags     : 0,
        ai_family    : AF_INET,
        ai_socktype  : SOCK_STREAM,
        ai_protocol  : IPPROTO_TCP,
        ai_addrlen   : 0,
        ai_addr      : nullptr,
        ai_canonname : nullptr,
        ai_next      : nullptr,
    };

    struct addrinfo *addrinfo;
    ret = getaddrinfo(HOST_STR.c_str(), PORT_STR.c_str(),
                      &addrinfo_hints, &addrinfo);

    DIE(ret != 0 || addrinfo == nullptr,
        "could not resolve '%s': %s", HOST_STR.c_str(), gai_strerror(ret));

    ret = ::connect(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    DIE(ret == -1, "connect");

    freeaddrinfo(addrinfo);
}

void Client::close() {
    int ret;

    if (sockfd == -1) {
        return;
    }

    ret = ::close(sockfd);
    DIE(ret == -1, "close");

    sockfd = -1;
}

bool Client::send(std::string const& d) {
    int ret;

    if (sockfd == -1) {
        connect();
    }

    std::size_t len = 0;
    std::size_t tot = d.length();

    while (len < tot) {
        ret = ::send(sockfd, d.c_str()+len, tot-len, MSG_NOSIGNAL);
        if (ret == -1) {
            DIE(errno != EPIPE, "send");
            close();
            return false;
        }

        len += ret;
    }

    return true;
}

bool Client::recv() {
    int ret;

    if (sockfd == -1) {
        connect();
    }

    // clear last message
    std::fill(buf.begin(), buf.end(), 0);
    resp.clear();

    std::size_t len = 0;
    std::size_t tot = buf.size();

    std::uint8_t* cur = buf.data()+len;
    std::uint8_t* end = buf.data()+tot;

    bool hdr = false;
    std::size_t hdr_len = 0;
    std::uint8_t* hdr_end = buf.data()+hdr_len;

    while (!hdr || len < tot) {
        if (len >= buf.size()) {
            buf.resize(buf.size() ? 2*buf.size() : 1);

            if (!hdr) {
                tot = buf.size();
            }

            cur = buf.data()+len;
            end = buf.data()+tot;

            hdr_end = buf.data()+hdr_len;
        }

        ret = ::recv(sockfd, cur, buf.size()-len, 0);
        if (ret == 0 || ret == -1) {
            DIE(ret == -1 && errno != ECONNRESET, "recv");
            close();
            return false;
        }

        if (!hdr) {
            hdr_end = find(cur, end, META_END);
        }

        if (!hdr && hdr_end != end) {
            hdr_end += META_END.length();
            hdr_len = hdr_end-buf.data();

            std::uint8_t* p = buf.data();
            std::uint8_t* p_next = find(p, hdr_end, META_TERM);

            std::istringstream iss(std::string(p, p_next));

            std::string proto_ver;
            std::uint16_t status_code;
            std::string status_text;

            iss >> proto_ver >> status_code >> status_text;

            DIE(proto_ver != PROTO_VER,
                "incompatible protocol version '%s'", proto_ver.c_str());

            resp["Status"] = status_code;

            do {
                p = p_next+META_TERM.length();
                p_next = find(p, hdr_end, META_TERM);

                if (p == p_next) { // no headers
                    break;
                }

                std::uint8_t* sep = find(p, p_next, HDR_SEP);

                std::string k(p, sep);
                std::string v(sep+HDR_SEP.length()+1, p_next);

                if (k == "Content-Length") {
                    resp[k] = std::stoi(v);
                } else if (k == "Set-Cookie") {
                    cookie = std::string(
                        v.begin(),
                        v.begin()+v.find(HDR_COOKIE_SEP)
                    );

                    resp[k] = v;
                } else {
                    resp[k] = v;
                }
            } while (!equal(p_next, META_END));

            tot = 0;

            tot += hdr_len;
            if (resp.count("Content-Length")) {
                tot += std::any_cast<int>(resp["Content-Length"]);
            }

            end = buf.data()+tot;

            hdr = true;
        }

        len += ret;

        cur += ret;
    }

    auto j = nlohmann::json::parse(hdr_end, end, nullptr, false);

    if (j.is_discarded()) {
        j = {
            {"error", std::string(hdr_end, end)},
        };
    }

    if (j.contains("token")) {
        jwt = j["token"];
    }

    resp["Content"] = j;

    return true;
}

void Client::clear() {
    cookie.clear();
    jwt.clear();
}

std::pair<std::uint16_t, nlohmann::json>
Client::get(std::string const& target) {
    std::ostringstream oss;

    meta_add_start(oss, "GET", target);
    meta_add_hdrs(oss);
    oss << META_TERM;

    std::string req = oss.str();

    while (!send(req) || !recv());

    return {
        std::any_cast<std::uint16_t>(resp["Status"]),
        std::any_cast<nlohmann::json>(resp["Content"]),
    };
}

std::pair<std::uint16_t, nlohmann::json>
Client::post(std::string const& target, nlohmann::json j) {
    std::ostringstream oss;

    std::string content = j.dump();

    meta_add_start(oss, "POST", target);
    meta_add_hdrs(oss);
    meta_add_content_hdrs(oss, content);
    oss << META_TERM;
    oss << content;

    std::string req = oss.str();

    while (!send(req) || !recv());

    return {
        std::any_cast<std::uint16_t>(resp["Status"]),
        std::any_cast<nlohmann::json>(resp["Content"]),
    };
}

std::pair<std::uint16_t, nlohmann::json>
Client::del(std::string const& target) {
    std::ostringstream oss;

    meta_add_start(oss, "DELETE", target);
    meta_add_hdrs(oss);
    oss << META_TERM;

    std::string req = oss.str();

    while (!send(req) || !recv());

    return {
        std::any_cast<std::uint16_t>(resp["Status"]),
        std::any_cast<nlohmann::json>(resp["Content"]),
    };
}
