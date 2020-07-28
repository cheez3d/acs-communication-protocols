#include "ether.h"

#include "arp.h"
#include "iface.h"
#include "ip.h"
#include "utils.h"

#include <string.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>

void ether_handle(struct msg *msg_in, struct msg *msg_out) {
    struct ethhdr *ethhdr_in = (struct ethhdr *)msg_in->frame;
    struct ethhdr *ethhdr_out = (struct ethhdr *)msg_out->frame;

    if (msg_in->status == MSG_OK)
    if (!ether_addr_eq((struct ether_addr *)ethhdr_in->h_dest,
                        if_ether_addr(msg_in->iface)))
    if (!ether_addr_broadcast((struct ether_addr *)ethhdr_in->h_dest)) {
        msg_out->status = MSG_DROPPED;
        return;
    }

    msg_out->len += sizeof(*ethhdr_out);

    switch (ntohs(ethhdr_in->h_proto)) {
    case ETHERTYPE_ARP:
        arp_handle(msg_in, msg_out);
        break;

    case ETHERTYPE_IP:
        ip_handle(msg_in, msg_out);
        break;
    }
}
