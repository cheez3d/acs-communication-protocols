#include "msg.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

#include <string.h>

#include <cinttypes>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <list>
#include <string>
#include <unordered_map>

static std::unordered_map<int, std::string> cl_ids;
static std::unordered_map<std::string, int> cl_fds;

static std::unordered_map<std::string,
                          std::list<union buffer>> sfs;

static std::unordered_map<std::string,
                          std::unordered_map<std::string, bool>> topics;

static std::unordered_map<std::string,
                          std::unordered_map<std::string, bool>> subs;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int ret;
    std::size_t len, cnt;

    fd_set sv_fds;
    int sv_fds_max = -1;
    FD_ZERO(&sv_fds);

    struct sockaddr_in sv_addr = {
        .sin_family = AF_INET,
        .sin_port = parse_port_str(argv[1]),
        .sin_addr = {INADDR_ANY},
        .sin_zero = {0},
    };

    int sv_fd_dgram = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sv_fd_dgram < 0, "socket");

    ret = setsockopt(sv_fd_dgram, SOL_SOCKET, SO_REUSEADDR, &(ret = 1), sizeof(ret));
    DIE(ret < 0, "setsockopt");

    ret = bind(sv_fd_dgram, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    DIE(ret < 0, "bind");

    FD_SET(sv_fd_dgram, &sv_fds);
    sv_fds_max = std::max(sv_fds_max, sv_fd_dgram);

    int sv_fd_stream = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sv_fd_stream < 0, "socket");

    ret = setsockopt(sv_fd_stream, IPPROTO_TCP, TCP_NODELAY, &(ret = 1), sizeof(ret));
    DIE(ret < 0, "setsockopt");

    // ret = setsockopt (sv_fd_stream, SOL_TCP, TCP_USER_TIMEOUT, &(ret = 5000), sizeof(ret));
    // DIE(ret < 0, "setsockopt");

    ret = setsockopt(sv_fd_stream, SOL_SOCKET, SO_REUSEADDR, &(ret = 1), sizeof(ret));
    DIE(ret < 0, "setsockopt");

    ret = bind(sv_fd_stream, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    DIE(ret < 0, "bind");

    ret = listen(sv_fd_stream, SOMAXCONN);
    DIE(ret < 0, "listen");

    FD_SET(sv_fd_stream, &sv_fds);
    sv_fds_max = std::max(sv_fds_max, sv_fd_stream);

    FD_SET(STDIN_FILENO, &sv_fds);
    sv_fds_max = std::max(sv_fds_max, STDIN_FILENO);

    union buffer u;

    while (true) {
        fd_set sv_fds_copy = sv_fds;
        ret = select(sv_fds_max+1, &sv_fds_copy, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(sv_fd_dgram, &sv_fds_copy)) {
            struct sockaddr_in cl_addr_dgram;
            socklen_t cl_addr_dgram_len = sizeof(cl_addr_dgram);

            // to handle non-null-terminated strings with < `MSG_DGRAM_DATA_LEN` chars
            std::memset(&u.msg_dgram.m.data, 0, MSG_DGRAM_DATA_LEN);

            ret = recvfrom(sv_fd_dgram, &u.msg_dgram.m, sizeof(u.msg_dgram.m), 0,
                           (struct sockaddr*)&cl_addr_dgram, &cl_addr_dgram_len);
            if (ret <= 0) {
                continue;
            }

            char topic[MSG_DGRAM_TOPIC_LEN+1] = {};
            std::strncpy(topic, u.msg_dgram.m.topic, MSG_DGRAM_TOPIC_LEN);

            // std::printf("Received data on '%s' from %s:%" PRIu16 ".\n",
            //             topic,
            //             inet_ntoa(cl_addr_dgram.sin_addr), cl_addr_dgram.sin_port);

            for (auto&& [cl_id, sf] : topics[topic]) {
                len = 0;

                len += sizeof(u.msg_dgram.m.topic);
                len += sizeof(u.msg_dgram.m.type);

                if (u.msg_dgram.m.type == msg_dgram::type::STRING) {
                    uint16_t str_len = strnlen(u.msg_dgram.m.data, MSG_DGRAM_DATA_LEN)+1;

                    if (str_len > MSG_DGRAM_DATA_LEN) {
                        str_len = MSG_DGRAM_DATA_LEN;
                    }

                    len += str_len;
                } else {
                    len += msg_dgram::type_size[u.msg_dgram.m.type];
                }

                u.msg_stream.hdr.len = htons(len);
                u.msg_stream.hdr.type = msg_stream::type::DATA;
                u.msg_stream.hdr.cl_dgram_addr = cl_addr_dgram;

                if (cl_fds.count(cl_id)) { // we can forward the message immediately
                    int cl_fd = cl_fds.at(cl_id);

                    len += sizeof(u.msg_stream.hdr);
                    cnt = 0;

                    while (cnt < len) {
                        ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                        if (ret < 0) {
                            cl_ids.erase(cl_fd);
                            cl_fds.erase(cl_id);

                            ret = close(cl_fd);
                            DIE(ret < 0, "close");
                            FD_CLR(cl_fd, &sv_fds);

                            std::printf("Client '%s' disconnected.\n", cl_id.c_str());
                            goto loop_end;
                        }
                        cnt += ret;
                    }

                    continue;
                }

                if (!sf) { // client did not subscribe with SF
                    continue;
                }

                sfs[cl_id].push_back(u);
            }

            FD_CLR(sv_fd_dgram, &sv_fds_copy);
        }

        if (FD_ISSET(sv_fd_stream, &sv_fds_copy)) {
            struct sockaddr_in cl_addr_stream;
            socklen_t cl_addr_stream_len = sizeof(cl_addr_stream);

            int cl_fd_stream = accept(sv_fd_stream,
                                      (struct sockaddr*)&cl_addr_stream, &cl_addr_stream_len);
            DIE(cl_fd_stream < 0, "accept");

            ret = setsockopt(cl_fd_stream, IPPROTO_TCP, TCP_NODELAY, &(ret = 1), sizeof(ret));
            DIE(ret < 0, "setsockopt");

            FD_SET(cl_fd_stream, &sv_fds);
            sv_fds_max = std::max(sv_fds_max, cl_fd_stream);

            FD_CLR(sv_fd_stream, &sv_fds_copy);
        }

        if (FD_ISSET(STDIN_FILENO, &sv_fds_copy)) {
            char* buf = std::fgets(u.buf, sizeof(u.buf), stdin);
            if (buf == NULL) {
                std::fprintf(stderr, "Missing command.\n");
                continue;
            }

            u.buf[std::strcspn(u.buf, "\n")] = '\0';

            char* cmd = std::strtok(u.buf, " ");

            if (cmd == NULL) {
                std::fprintf(stderr, "Missing command.\n");
                continue;
            }

            if (std::strcmp(cmd, CMD_EXIT_STR) == 0) {
                break;
            }

            std::fprintf(stderr, "Unknown command '%s'.\n", cmd);

            FD_CLR(STDIN_FILENO, &sv_fds_copy);
        }

        for (int cl_fd = 0; cl_fd <= sv_fds_max; ++cl_fd) {
            if (!FD_ISSET(cl_fd, &sv_fds_copy)) {
                continue;
            }

            len = sizeof(u.msg_stream.hdr); // make sure whole header is received
            cnt = 0;

            for (bool hdr = false; cnt < len; ) {
                ret = recv(cl_fd, &u.buf[cnt], len-cnt, 0);
                if (ret <= 0) {
                    std::string& cl_id = cl_ids[cl_fd];

                    cl_ids.erase(cl_fd);
                    cl_fds.erase(cl_id);

                    if (ret == 0) {
                        ret = shutdown(cl_fd, SHUT_RDWR);
                        DIE(ret < 0, "shutdown");
                    }

                    ret = close(cl_fd);
                    DIE(ret < 0, "close");
                    FD_CLR(cl_fd, &sv_fds);

                    std::printf("Client '%s' disconnected.\n", cl_id.c_str());

                    goto cl_loop_end;
                }
                cnt += ret;

                // after header, make sure whole message is received
                if (!hdr && cnt >= len) {
                    u.msg_stream.hdr.len = ntohs(u.msg_stream.hdr.len);
                    len += u.msg_stream.hdr.len;
                    hdr = true;
                }
            }

            switch (u.msg_stream.hdr.type) {
            case msg_stream::type::NAME: {
                char cl_id[CL_ID_LEN+1] = {};
                std::strncpy(cl_id, u.msg_stream.data, CL_ID_LEN);

                if (cl_fds.count(cl_id)) {
                    std::fprintf(stderr, "Client '%s' already connected.\n", cl_id);

                    ret = shutdown(cl_fd, SHUT_RD);
                    DIE(ret < 0, "shutdown");

                    len = 0;

                    std::sprintf(&u.msg_stream.data[len], "Already connected.\n");
                    len += std::strlen(&u.msg_stream.data[len])+1;

                    u.msg_stream.hdr.len = htons(len);
                    u.msg_stream.hdr.type = msg_stream::type::RESPONSE;

                    len += sizeof(u.msg_stream.hdr);
                    cnt = 0;

                    while (cnt < len) {
                        ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                        if (ret < 0) {
                            cl_ids.erase(cl_fd);
                            cl_fds.erase(cl_id);

                            ret = close(cl_fd);
                            DIE(ret < 0, "close");
                            FD_CLR(cl_fd, &sv_fds);

                            std::printf("Client '%s' disconnected.\n", cl_id);
                            goto cl_loop_end;
                        }
                        cnt += ret;
                    }

                    ret = shutdown(cl_fd, SHUT_WR);
                    DIE(ret < 0, "shutdown");

                    ret = close(cl_fd);
                    DIE(ret < 0, "close");
                    FD_CLR(cl_fd, &sv_fds);

                    continue;
                }

                cl_ids[cl_fd] = cl_id;
                cl_fds[cl_id] = cl_fd;

                struct sockaddr_in cl_addr;
                socklen_t cl_addr_len = sizeof(cl_addr);

                getpeername(cl_fd, (struct sockaddr*)&cl_addr, &cl_addr_len);

                std::printf("New client '%s' connected from %s:%" PRIu16 ".\n", cl_id,
                            inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));

                if (!sfs.count(cl_id)) {
                    continue;
                }

                for (auto&& u : sfs[cl_id]) {
                    len = sizeof(u.msg_stream.hdr)+ntohs(u.msg_stream.hdr.len);
                    cnt = 0;

                    while (cnt < len) {
                        ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                        if (ret < 0) {
                            cl_ids.erase(cl_fd);
                            cl_fds.erase(cl_id);

                            ret = close(cl_fd);
                            DIE(ret < 0, "close");
                            FD_CLR(cl_fd, &sv_fds);

                            std::printf("Client '%s' disconnected.\n", cl_id);
                            goto cl_loop_end;
                        }
                        cnt += ret;
                    }
                }

                sfs.erase(cl_id);

                break;
            }

            case msg_stream::type::SUBSCRIBE: {
                std::string& cl_id = cl_ids[cl_fd];

                cnt = 0;

                char* topic = &u.msg_stream.data[cnt];
                cnt += std::strlen(topic)+1;

                if (topics[topic].count(cl_id)) {
                    std::fprintf(stderr, "Client '%s' already subscribed to '%s'.\n",
                                 cl_id.c_str(), topic);

                    len = 0;

                    std::sprintf(&u.msg_stream.data[len], "Already subscribed to topic.\n");
                    len += std::strlen(&u.msg_stream.data[len])+1;

                    u.msg_stream.hdr.len = htons(len);
                    u.msg_stream.hdr.type = msg_stream::type::RESPONSE;

                    len += sizeof(u.msg_stream.hdr);
                    cnt = 0;

                    while (cnt < len) {
                        ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                        if (ret < 0) {
                            cl_ids.erase(cl_fd);
                            cl_fds.erase(cl_id);

                            ret = close(cl_fd);
                            DIE(ret < 0, "close");
                            FD_CLR(cl_fd, &sv_fds);

                            std::printf("Client '%s' disconnected.\n", cl_id.c_str());
                            goto cl_loop_end;
                        }
                        cnt += ret;
                    }

                    continue;
                }

                uint8_t sf;
                memcpy(&sf, &u.msg_stream.data[cnt], sizeof(sf));

                topics[topic][cl_id] = sf;
                subs[cl_id][topic] = sf;

                std::printf("Client '%s' subscribed to '%s' with%s SF.\n",
                            cl_id.c_str(), topic, sf ? "" : "out");

                len = 0;

                std::sprintf(&u.msg_stream.data[len], "Subscribed to topic.\n");
                len += std::strlen(&u.msg_stream.data[len])+1;

                u.msg_stream.hdr.len = htons(len);
                u.msg_stream.hdr.type = msg_stream::type::RESPONSE;

                len += sizeof(u.msg_stream.hdr);
                cnt = 0;

                while (cnt < len) {
                    ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                    if (ret < 0) {
                        cl_ids.erase(cl_fd);
                        cl_fds.erase(cl_id);

                        ret = close(cl_fd);
                        DIE(ret < 0, "close");
                        FD_CLR(cl_fd, &sv_fds);

                        std::printf("Client '%s' disconnected.\n", cl_id.c_str());
                        goto cl_loop_end;
                    }
                    cnt += ret;
                }

                break;
            }

            case msg_stream::type::UNSUBSCRIBE: {
                std::string& cl_id = cl_ids[cl_fd];

                cnt = 0;

                char* topic = &u.msg_stream.data[cnt];

                if (!topics[topic].count(cl_id)) {
                    std::fprintf(stderr, "Client '%s' not subscribed to '%s'.\n",
                                 cl_id.c_str(), topic);

                    len = 0;

                    std::sprintf(&u.msg_stream.data[len], "Not subscribed to topic.\n");
                    len += std::strlen(&u.msg_stream.data[len])+1;

                    u.msg_stream.hdr.len = htons(len);
                    u.msg_stream.hdr.type = msg_stream::type::RESPONSE;

                    len += sizeof(u.msg_stream.hdr);
                    cnt = 0;

                    while (cnt < len) {
                        ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                        if (ret < 0) {
                            cl_ids.erase(cl_fd);
                            cl_fds.erase(cl_id);

                            ret = close(cl_fd);
                            DIE(ret < 0, "close");
                            FD_CLR(cl_fd, &sv_fds);

                            std::printf("Client '%s' disconnected.\n", cl_id.c_str());
                            goto cl_loop_end;
                        }
                        cnt += ret;
                    }

                    continue;
                }

                topics[topic].erase(cl_id);
                subs[cl_id].erase(topic);

                std::printf("Client '%s' unsubscribed from '%s'.\n",
                            cl_id.c_str(), topic);

                len = 0;

                std::sprintf(&u.msg_stream.data[len], "Unsubscribed from topic.\n");
                len += std::strlen(&u.msg_stream.data[len])+1;

                u.msg_stream.hdr.len = htons(len);
                u.msg_stream.hdr.type = msg_stream::type::RESPONSE;

                len += sizeof(u.msg_stream.hdr);
                cnt = 0;

                while (cnt < len) {
                    ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                    if (ret < 0) {
                        cl_ids.erase(cl_fd);
                        cl_fds.erase(cl_id);

                        ret = close(cl_fd);
                        DIE(ret < 0, "close");
                        FD_CLR(cl_fd, &sv_fds);

                        std::printf("Client '%s' disconnected.\n", cl_id.c_str());
                        goto cl_loop_end;
                    }
                    cnt += ret;
                }

                break;
            }

            default:
                std::fprintf(stderr, "Malformed message received from client '%s'.\n",
                             cl_ids[cl_fd].c_str());
            }

        cl_loop_end:
            FD_CLR(cl_fd, &sv_fds_copy);
        }

    loop_end:
        ;
    }

    FD_CLR(STDIN_FILENO, &sv_fds);

    ret = close(sv_fd_stream);
    DIE(ret < 0, "close");
    FD_CLR(sv_fd_stream, &sv_fds);

    ret = close(sv_fd_dgram);
    DIE(ret < 0, "close");
    FD_CLR(sv_fd_dgram, &sv_fds);

    for (int cl_fd = 0; cl_fd <= sv_fds_max; ++cl_fd) {
        if (!FD_ISSET(cl_fd, &sv_fds)) {
            continue;
        }

        ret = shutdown(cl_fd, SHUT_RDWR);
        DIE(ret < 0, "shutdown");

        ret = close(cl_fd);
        DIE(ret < 0, "close");
        FD_CLR(cl_fd, &sv_fds);
    }

    return EXIT_SUCCESS;
}
