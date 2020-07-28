#pragma once

#include <netinet/in.h>

#include <cstdint>

#define MSG_DGRAM_TOPIC_LEN 50
#define MSG_DGRAM_DATA_LEN 1500

struct __attribute__((__packed__)) msg_dgram {
    enum type : uint8_t {
        INT        = 0,
        SHORT_REAL = 1,
        FLOAT      = 2,
        STRING     = 3,
    };

    static constexpr const uint16_t type_size[] = {
        [INT]        = sizeof(uint8_t)+sizeof(uint32_t),
        [SHORT_REAL] = sizeof(uint16_t),
        [FLOAT]      = sizeof(uint8_t)+sizeof(uint32_t)+sizeof(uint8_t),
        [STRING]     = MSG_DGRAM_DATA_LEN,
    };

    static constexpr const char* type_str[] = {
        [INT]        = "INT",
        [SHORT_REAL] = "SHORT_REAL",
        [FLOAT]      = "FLOAT",
        [STRING]     = "STRING",
    };

    char topic[MSG_DGRAM_TOPIC_LEN];

    enum type type;

    char data[MSG_DGRAM_DATA_LEN];
};

struct __attribute__((__packed__)) msg_stream  {
    enum type : uint8_t {
        NAME        = 0,
        SUBSCRIBE   = 1,
        UNSUBSCRIBE = 2,
        RESPONSE    = 3,
        DATA        = 4,
    };

    struct __attribute__((__packed__)) {
        uint16_t len;
        enum type type;
        struct sockaddr_in cl_dgram_addr;
    } hdr;

    char data[sizeof(struct msg_dgram)];
};

union buffer {
    struct {
        char unused[sizeof(msg_stream)-sizeof(msg_dgram)];
        msg_dgram m;
    } msg_dgram;

    struct msg_stream msg_stream;

    char buf[sizeof(msg_stream)];
};
