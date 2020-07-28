#ifndef RTABLE_H
#define RTABLE_H

#include <stddef.h>
#include <netinet/in.h>

struct rtable_node {
    struct rtable_e *entry;
    struct rtable_node *sub[2];
};

struct rtable_e {
    size_t iface;
    struct in_addr next_hop;
};

struct rtable {
    struct rtable_node *root;
    size_t size;
};

struct rtable_e * rtable_lookup(struct in_addr addr);

void rtable_init(void);
void rtable_term(void);

#endif // RTABLE_H
