#include "icmp.h"
#include "utils.h"

#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>

void icmp_send(struct msg *msg_in, struct msg *msg_out, uint8_t type) {
    size_t old_len = msg_in->len;
    msg_in->len = msg_out->len;

    struct icmphdr *icmphdr = (struct icmphdr *)(msg_in->frame+msg_in->len);

    struct iphdr *iphdr = (struct iphdr *)icmphdr-1;

    char *icmpdata = (char *)(icmphdr+1);
    size_t icmpdata_len = 0;

    msg_in->len += sizeof(*icmphdr);

    switch (type) {
    case ICMP_DEST_UNREACH:
    case ICMP_TIME_EXCEEDED: {
        icmpdata_len = sizeof(*iphdr)+8;
        memcpy(icmpdata, iphdr, icmpdata_len);

        icmphdr->code = 0;

        msg_in->len += icmpdata_len;

        break;
    }

    case ICMP_ECHOREPLY: {
        icmpdata_len = msg_in->frame+old_len-icmpdata;
        // no need to copy, sending reply with same contents

        icmphdr->code = 0;

        msg_in->len = old_len;

        break;
    }
    }

    icmphdr->type = type;

    icmphdr->checksum = 0x0000;

    icmphdr->checksum
        = checksum(icmphdr, sizeof(*icmphdr)+icmpdata_len);

    iphdr->version = IPVERSION;
    iphdr->ihl = sizeof(*iphdr)/sizeof(uint32_t);
    iphdr->tos = 0;
    iphdr->tot_len = htons(icmpdata+icmpdata_len-(char *)iphdr);
    iphdr->id = htons(getpid());
    iphdr->frag_off = 0x0000;
    iphdr->ttl = IPDEFTTL+1;
    iphdr->protocol = IPPROTO_ICMP;
    iphdr->check = 0x0000;
    iphdr->daddr = iphdr->saddr;
    iphdr->saddr = if_in_addr(msg_in->iface).s_addr;

    iphdr->check = checksum(iphdr, sizeof(*iphdr));
}

void icmp_handle(struct msg *msg_in, struct msg *msg_out) {
    struct icmphdr *icmphdr = (struct icmphdr *)(msg_in->frame+msg_out->len);

    switch (icmphdr->type) {
    case ICMP_ECHO: {
        if (icmphdr->code != 0) {
            msg_out->status = MSG_DROPPED;
            return;
        }

        icmp_send(msg_in, msg_out, ICMP_ECHOREPLY);

        break;
    }

    default: {
        msg_out->status = MSG_DROPPED;
        return;
    }
    }
}
