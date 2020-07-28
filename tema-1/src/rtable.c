#include "rtable.h"

#include "list.h"
#include "utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static const char *RTABLE_FILE = "rtable.txt";

static struct rtable *rtable;

enum rtable_addr {
    RTABLE_ADDR_PREFIX,
    RTABLE_ADDR_NEXT_HOP,
    RTABLE_ADDR_MASK,
    
    RTABLE_ADDR_CNT,
};

static struct rtable_node * rtable_node_create(void) {
    struct rtable_node *node = calloc(1, sizeof(*node));

    node->entry = NULL;
    for (size_t i = 0; i < sizeof(node->sub)/sizeof(*node->sub); ++i) {
        node->sub[i] = NULL;
    }

    return node;
}

static void rtable_node_destroy(struct rtable_node *node) {
    assert(node != NULL);

    if (node->entry != NULL) {
        free(node->entry);
    }

    free(node);
}

static struct rtable * rtable_create(void) {
    struct rtable *rtable = calloc(1, sizeof(*rtable));

    rtable->root = rtable_node_create();
    rtable->size = 0;

    return rtable;
}

static void rtable_destroy_rec(struct rtable_node *node) {
    if (node == NULL) {
        return;
    }

    for (size_t i = 0; i < sizeof(node->sub)/sizeof(*node->sub); ++i) {
        rtable_destroy_rec(node->sub[i]);
    }

    rtable_node_destroy(node);
}

static void rtable_destroy(struct rtable *rtable) {
    assert(rtable != NULL);

    rtable_destroy_rec(rtable->root);

    free(rtable);
}

struct rtable_e * rtable_lookup(struct in_addr addr) {
    addr.s_addr = ntohl(addr.s_addr);

    struct rtable_node *node = rtable->root;
    struct rtable_e *e = NULL;

    for (size_t i = 0; i < 8 * sizeof(addr) && node != NULL; ++i) {
        if (node->entry != NULL) {
            e = node->entry;
        }

        size_t bit = _rotl(addr.s_addr, 1) & 1;
        addr.s_addr <<= 1;

        node = node->sub[bit];
    }

    return e;
}

static struct rtable_node *
rtable_add_rec(struct rtable_node *node, struct in_addr *addrs, size_t iface) {
    assert(node != NULL);

    if (addrs[RTABLE_ADDR_MASK].s_addr == INADDR_ANY) {
        assert(node->entry == NULL);

        struct rtable_e *e = node->entry = calloc(1, sizeof(*e));

        e->iface = iface;
        e->next_hop = addrs[RTABLE_ADDR_NEXT_HOP];

        return node;
    }

    size_t bit = _rotl(addrs[RTABLE_ADDR_PREFIX].s_addr, 1) & 1;

    addrs[RTABLE_ADDR_PREFIX].s_addr <<= 1;
    addrs[RTABLE_ADDR_MASK].s_addr <<= 1;

    if (node->sub[bit] == NULL) node->sub[bit] = rtable_node_create();

    return rtable_add_rec(node->sub[bit], addrs, iface);
}

static struct rtable_node *
rtable_add(struct rtable *rtable, struct in_addr *addrs, size_t iface) {
    assert(rtable != NULL);

    struct rtable_node *node = rtable_add_rec(rtable->root, addrs, iface);

    ++rtable->size;

    return node;
}

void rtable_init(void) {
    int ret;

    rtable = rtable_create();

    FILE *f = fopen(RTABLE_FILE, "r");

    while (true) {
        char *addr_str = NULL;

        struct in_addr addrs[RTABLE_ADDR_CNT];

        for (enum rtable_addr i = 0; i < RTABLE_ADDR_CNT; ++i) {
            if (fscanf(f, "%ms", &addr_str) < 1) goto eof;

            if (i == RTABLE_ADDR_NEXT_HOP) {
                ret = inet_pton(AF_INET, addr_str, &addrs[i].s_addr);
                DIE(ret <= 0, "invalid address %s", addr_str);
            } else {
                addrs[i].s_addr = inet_network(addr_str);
            }

            free(addr_str);
        }

        size_t iface;
        if (fscanf(f, "%zu", &iface) < 1) goto eof;

        rtable_add(rtable, addrs, iface);
    }

eof:
    fclose(f);
}

void rtable_term(void) {
    rtable_destroy(rtable);
}
