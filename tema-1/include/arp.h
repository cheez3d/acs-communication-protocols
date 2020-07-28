#ifndef ARP_H
#define ARP_H

#include "iface.h"
#include "list.h"
#include "rtable.h"

#include <stdbool.h>
#include <stddef.h>
#include <netinet/ether.h>
#include <netinet/in.h>

struct arp_cache_e {
    size_t iface;
    struct in_addr in_addr;
    struct ether_addr ether_addr;
};

struct arp_queue_e {
    struct rtable_e *rtable_e;
    struct msg msg;
};

struct arp_cache_e * arp_cache_lookup(size_t iface, struct in_addr addr);

void arp_req_send(struct msg *msg_out, struct in_addr addr);

void arp_handle(struct msg *msg_in, struct msg *msg_out);

struct list_node *arp_queue_add(struct rtable_e *rtable_e, struct msg *msg);
bool arp_queue_process(struct msg *msg);

void arp_init(void);
void arp_term(void);

#endif // ARP_H
