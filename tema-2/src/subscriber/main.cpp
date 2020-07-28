#include "msg.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <unistd.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <algorithm>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::fprintf(stderr, "usage: %s <cl_id> <sv_addr> <sv_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int ret;
    std::size_t len, cnt;

    char* cl_id = proc_cl_id(argv[1]);

    fd_set cl_fds;
    int fds_max = -1;
    FD_ZERO(&cl_fds);

    struct sockaddr_in cl_addr = {
        .sin_family = AF_INET,
        .sin_port = parse_port_str(argv[3]),
        .sin_addr = {parse_addr_str(argv[2])},
        .sin_zero = {0},
    };

    int cl_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(cl_fd < 0, "socket");

    ret = setsockopt(cl_fd, IPPROTO_TCP, TCP_NODELAY, &(ret = 1), sizeof(ret));
    DIE(ret < 0, "setsockopt");

    ret = connect(cl_fd, (struct sockaddr*)&cl_addr, sizeof(cl_addr));
    DIE(ret < 0, "connect");

    FD_SET(cl_fd, &cl_fds);
    fds_max = std::max(fds_max, cl_fd);

    FD_SET(STDIN_FILENO, &cl_fds);
    fds_max = std::max(fds_max, STDIN_FILENO);

    union buffer u;

    len = 0;

    memcpy(u.msg_stream.data, cl_id, strlen(cl_id)+1);
    len += std::strlen(cl_id)+1;

    u.msg_stream.hdr.len = htons(len);
    u.msg_stream.hdr.type = msg_stream::type::NAME;

    len += sizeof(u.msg_stream.hdr);
    cnt = 0;

    while (cnt < len) {
        ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
        if (ret < 0) {
            goto exit;
        }
        cnt += ret;
    }

    while (true) {
        fd_set cl_fds_copy = cl_fds;
        ret = select(fds_max+1, &cl_fds_copy, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(cl_fd, &cl_fds_copy)) {
            len = sizeof(u.msg_stream.hdr); // make sure whole header is received
            cnt = 0;

            for (bool hdr = false; cnt < len; ) {
                ret = recv(cl_fd, &u.buf[cnt], len-cnt, 0);
                if (ret <= 0) {
                    goto exit;
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
            case msg_stream::type::RESPONSE: {
                std::printf("%s", u.msg_stream.data);
                break;
            }

            case msg_stream::type::DATA: {
                char topic[MSG_DGRAM_TOPIC_LEN+1] = {};
                std::strncpy(topic, u.msg_dgram.m.topic, MSG_DGRAM_TOPIC_LEN);

                std::printf("%s:%" PRIu16 " - %s - %s - ",
                            inet_ntoa(u.msg_stream.hdr.cl_dgram_addr.sin_addr),
                            ntohs(u.msg_stream.hdr.cl_dgram_addr.sin_port),
                            topic,
                            msg_dgram::type_str[u.msg_dgram.m.type]);

                cnt = 0;

                switch (u.msg_dgram.m.type) {
                case msg_dgram::type::INT: {
                    uint8_t s;
                    std::memcpy(&s, &u.msg_dgram.m.data[cnt], sizeof(s));
                    cnt += sizeof(s);

                    uint32_t v;
                    std::memcpy(&v, &u.msg_dgram.m.data[cnt], sizeof(v));

                    v = ntohl(v);

                    std::printf("%s%" PRIu32, s ? "-" : "", v);

                    break;
                }

                case msg_dgram::type::SHORT_REAL: {
                    uint16_t v;
                    std::memcpy(&v, &u.msg_dgram.m.data[cnt], sizeof(v));

                    v = ntohs(v);

                    std::printf("%" PRIu16 ".%02" PRIu16,
                                v/(uint16_t)100, v%(uint16_t)100);

                    break;
                }

                case msg_dgram::type::FLOAT: {
                    uint8_t s;
                    std::memcpy(&s, &u.msg_dgram.m.data[cnt], sizeof(s));
                    cnt += sizeof(s);

                    uint32_t v;
                    std::memcpy(&v, &u.msg_dgram.m.data[cnt], sizeof(v));
                    cnt += sizeof(v);

                    v = ntohl(v);

                    uint8_t e;
                    std::memcpy(&e, &u.msg_dgram.m.data[cnt], sizeof(e));
                    cnt += sizeof(e);

                    if (e == 0) {
                        std::printf("%s%" PRIu32, s ? "-" : "", v);
                    } else {
                        uint64_t pow = 1;
                        for (uint8_t i = 0; i < e; ++i) pow *= 10;

                        std::printf("%s%" PRIu32 ".%0*" PRIu32,
                                    s ? "-" : "",
                                    (uint32_t)(v/pow), e, (uint32_t)(v%pow));
                    }

                    break;
                }

                case msg_dgram::type::STRING: {
                    char str[MSG_DGRAM_DATA_LEN+1] = {};
                    std::strncpy(str, &u.msg_dgram.m.data[cnt], MSG_DGRAM_DATA_LEN);

                    std::printf("%s", str);

                    break;
                }

                default:
                    std::fprintf(stderr, "Malformed data received from server.\n");
                    continue;
                }

                std::printf("\n");

                break;
            }

            default:
                std::fprintf(stderr, "Malformed message received from server.\n");
                continue;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &cl_fds_copy)) {
            char* buf = std::fgets(u.buf, sizeof(u.buf), stdin);
            if (buf == NULL) {
                std::fprintf(stderr, "Missing command.\n");
                continue;
            }

            u.buf[std::strcspn(u.buf, "\n")] = '\0';

            char* cmd_buf = std::strtok(u.buf, " ");

            if (cmd_buf == NULL) {
                std::fprintf(stderr, "Missing command.\n");
                continue;
            }

            if (std::strcmp(cmd_buf, CMD_EXIT_STR) == 0) {
                ret = shutdown(cl_fd, SHUT_WR);
                DIE(ret < 0, "shutdown");
                continue;
            }

            if (std::strcmp(cmd_buf, CMD_SUBSCRIBE_STR) == 0) {
                char* topic_buf = std::strtok(NULL, " ");

                if (topic_buf == NULL) {
                    std::fprintf(stderr, "Missing topic.\n");
                    continue;
                }

                char topic[MSG_DGRAM_TOPIC_LEN+1] = {};
                std::strncpy(topic, topic_buf, MSG_DGRAM_TOPIC_LEN);

                char* sf_buf = std::strtok(NULL, " ");

                if (sf_buf == NULL) {
                    std::fprintf(stderr, "Missing SF.\n");
                    continue;
                } else if (std::strcmp(sf_buf, "0") != 0 &&
                           std::strcmp(sf_buf, "1") != 0)
                {
                    std::fprintf(stderr, "Invalid SF '%s'.\n", sf_buf);
                    continue;
                }

                uint8_t sf = sf_buf[0] == '1';

                len = 0;

                std::memcpy(&u.msg_stream.data[len], topic, std::strlen(topic)+1);
                len += std::strlen(topic)+1;

                std::memcpy(&u.msg_stream.data[len], &sf, sizeof(sf));
                len += sizeof(sf);

                u.msg_stream.hdr.type = msg_stream::type::SUBSCRIBE;
            } else if (std::strcmp(cmd_buf, CMD_UNSUBSCRIBE_STR) == 0) {
                char* topic_buf = std::strtok(NULL, " ");

                if (topic_buf == NULL) {
                    std::fprintf(stderr, "Missing topic.\n");
                    continue;
                }

                char topic[MSG_DGRAM_TOPIC_LEN+1] = {};
                std::strncpy(topic, topic_buf, MSG_DGRAM_TOPIC_LEN);

                len = 0;

                std::memcpy(&u.msg_stream.data[len], topic, std::strlen(topic)+1);
                len += std::strlen(topic)+1;

                u.msg_stream.hdr.type = msg_stream::type::UNSUBSCRIBE;
            } else {
                std::fprintf(stderr, "Unknown command '%s'.\n", cmd_buf);
                continue;
            }

            u.msg_stream.hdr.len = htons(len);

            len += sizeof(u.msg_stream.hdr);
            cnt = 0;

            while (cnt < len) {
                ret = send(cl_fd, &u.buf[cnt], len-cnt, 0);
                if (ret < 0) {
                    goto exit;
                }
                cnt += ret;
            }
        }
    }

exit:
    ret = close(cl_fd);
    DIE(ret < 0, "close");

    std::puts("Disconnected.");

    return EXIT_SUCCESS;
}
