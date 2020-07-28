#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#define DIE(cond, msg, ...) \
    do { \
        if ((cond)) { \
            error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
        } \
    } while (0)

static inline bool
ether_addr_eq(struct ether_addr *addr_fst, struct ether_addr *addr_snd) {
    return memcmp(addr_fst, addr_snd, sizeof(*addr_fst)) == 0;
}

static inline bool ether_addr_broadcast(struct ether_addr *addr) {
    return (*((uint16_t *)addr+0) &
            *((uint16_t *)addr+1) &
            *((uint16_t *)addr+2)) == 0xffff;
}

static inline bool
in_addr_eq(struct in_addr addr_fst, struct in_addr addr_snd) {
    return addr_fst.s_addr == addr_snd.s_addr;
}

static inline uint16_t checksum(void *data, size_t len) {
    assert(data != NULL);

    uint16_t *word = (uint16_t *)data;
    uint32_t sum = 0;

    while (len > 1) {
        sum += ntohs(*word++);
        len -= 2;
    }

    // odd `len`
    if (len == 1) {
        sum += (uint8_t)ntohs(*word);
    }

    sum = (uint16_t)sum+(sum >> 16);
    sum += (sum >> 16);

    return htons(~(uint16_t)sum);
}

static inline uint16_t
checksum_inc(uint16_t new, uint16_t old, uint16_t checksum) {
    new = ntohs(new);
    old = ~ntohs(old);
    checksum = ~ntohs(checksum);

    uint32_t sum = (uint32_t)checksum+old+new;
    sum = (uint16_t)sum+(sum >> 16);

    return htons(~(uint16_t)sum);
}

#endif // UTILS_H
