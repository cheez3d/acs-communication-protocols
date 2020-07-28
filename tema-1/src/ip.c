#include "ip.h"

#include "arp.h"
#include "icmp.h"
#include "iface.h"
#include "rtable.h"
#include "utils.h"

#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <sys/types.h>

void ip_handle(struct msg *msg_in, struct msg *msg_out) {
    struct iphdr *iphdr_in = (struct iphdr *)(msg_in->frame+msg_out->len);
    struct iphdr *iphdr_out = (struct iphdr *)(msg_out->frame+msg_out->len);

    struct ethhdr *ethhdr_out = (struct ethhdr *)iphdr_out-1;

    msg_out->len += sizeof(*iphdr_out);

    if (iphdr_in->version != IPVERSION) {
        msg_out->status = MSG_DROPPED;
        return;
    }

    if (checksum(iphdr_in, iphdr_in->ihl*sizeof(uint32_t)) != 0x0000) {
        msg_out->status = MSG_DROPPED;
        return;
    }

    if (in_addr_eq(*(struct in_addr *)&iphdr_in->daddr,
                   if_in_addr(msg_in->iface)))
    {
        switch (iphdr_in->protocol) {
        case IPPROTO_ICMP:
            icmp_handle(msg_in, msg_out);
            break;

        default: {
            msg_out->status = MSG_DROPPED;
            return;
        }
        }
    }

    if (iphdr_in->ttl <= 1) {
        icmp_send(msg_in, msg_out, ICMP_TIME_EXCEEDED);
    }

    struct rtable_e *rtable_e
        = rtable_lookup(*(struct in_addr *)&iphdr_in->daddr);

    if (rtable_e == NULL) {
        icmp_send(msg_in, msg_out, ICMP_DEST_UNREACH);

        rtable_e = rtable_lookup(*(struct in_addr *)&iphdr_in->daddr);

        if (rtable_e == NULL) { // this should never be reached
            msg_out->status = MSG_DROPPED;
            return;
        }
    }

    msg_out->iface = rtable_e->iface;

    struct arp_cache_e *arp_e
        = arp_cache_lookup(rtable_e->iface, rtable_e->next_hop);

    if (arp_e == NULL) {
        msg_in->status = MSG_QUEUED;
        arp_queue_add(rtable_e, msg_in);

        msg_out->len = (char *)(ethhdr_out+1)-msg_out->frame;
        arp_req_send(msg_out, rtable_e->next_hop);

        return;
    }

    memcpy(iphdr_out, iphdr_in, sizeof(*iphdr_out));

    --iphdr_out->ttl;

    iphdr_out->check
        = checksum_inc(*(uint16_t *)&iphdr_out->ttl,
                       *(uint16_t *)&iphdr_in->ttl,
                       iphdr_in->check);

    memcpy(msg_out->frame+msg_out->len,
           msg_in->frame+msg_out->len,
           msg_in->len-msg_out->len);

    msg_out->len = msg_in->len;

    memcpy(ethhdr_out->h_dest,
           &arp_e->ether_addr,
           sizeof(ethhdr_out->h_dest));

    memcpy(ethhdr_out->h_source,
           if_ether_addr(msg_out->iface),
           sizeof(ethhdr_out->h_source));

    ethhdr_out->h_proto = htons(ETHERTYPE_IP);
}
