#include "arp.h"

#include "iface.h"
#include "list.h"
#include "utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

static struct list *arp_cache[IF_CNT];
static struct arp_cache_e *arp_cache_update;
static struct list *arp_queue;

static struct list_node *
arp_cache_lookup_node(size_t iface, struct in_addr addr) {
    assert(iface < IF_CNT);

    struct list *cache = arp_cache[iface];

    #define N struct list_node

    for (N *n = list_fst(cache); n != cache->back; n = n->next) {
        struct arp_cache_e *e = (struct arp_cache_e *)n->data;

        if (in_addr_eq(e->in_addr, addr)) {
            return n;
        }
    }

    #undef N

    return NULL;
}

struct arp_cache_e * arp_cache_lookup(size_t iface, struct in_addr addr) {
    assert(iface < IF_CNT);

    struct list_node *node = arp_cache_lookup_node(iface, addr);

    if (node != NULL) {
        return (struct arp_cache_e *)node->data;
    }

    return NULL;
}

static struct list_node *
arp_cache_add(size_t iface,
              struct in_addr in_addr,
              struct ether_addr *ether_addr)
{
    assert(iface < IF_CNT);
    assert(ether_addr != NULL);

    struct list_node *node = arp_cache_lookup_node(iface, in_addr);
    struct arp_cache_e *e;

    if (node == NULL) {
        e = calloc(1, sizeof(*e));
        e->iface = iface;
        e->in_addr = in_addr;

        node = list_add_fst(arp_cache[iface], e);
    } else {
        e = (struct arp_cache_e *)node->data;
    }

    if (!ether_addr_eq(&e->ether_addr, ether_addr)) {
        memcpy(&e->ether_addr, ether_addr, sizeof(*ether_addr));
    }

    return node;
}

void arp_req_send(struct msg *msg_out, struct in_addr addr) {
    assert(msg_out != NULL);

    struct ether_arp *req = (struct ether_arp *)(msg_out->frame+msg_out->len);

    req->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
    req->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
    req->ea_hdr.ar_hln = sizeof(req->arp_sha);
    req->ea_hdr.ar_pln = sizeof(req->arp_spa);
    req->ea_hdr.ar_op = htons(ARPOP_REQUEST);

    memcpy(req->arp_sha, if_ether_addr(msg_out->iface), sizeof(req->arp_sha));
    *(struct in_addr *)req->arp_spa = if_in_addr(msg_out->iface);

    memset(req->arp_tha, 0xff, sizeof(req->arp_tha));
    *(struct in_addr *)req->arp_tpa = addr;

    msg_out->len += sizeof(*req);

    struct ethhdr *ethhdr_out = (struct ethhdr *)req-1;

    memcpy(ethhdr_out->h_dest,
           req->arp_tha,
           sizeof(ethhdr_out->h_dest));

    memcpy(ethhdr_out->h_source,
           req->arp_sha,
           sizeof(ethhdr_out->h_source));

    ethhdr_out->h_proto = htons(ETHERTYPE_ARP);
}

static void arp_reply_send(struct msg *msg_in, struct msg *msg_out) {
    assert(msg_in != NULL);
    assert(msg_out != NULL);

    struct ether_arp *req = (struct ether_arp *)(msg_in->frame+msg_out->len);
    struct ether_arp *reply = (struct ether_arp *)(msg_out->frame+msg_out->len);

    memcpy(&reply->ea_hdr, &req->ea_hdr, sizeof(req->ea_hdr));
    reply->ea_hdr.ar_op = htons(ARPOP_REPLY);

    memcpy(reply->arp_sha,
           if_ether_addr(msg_out->iface),
           sizeof(reply->arp_sha));

    *(struct in_addr *)reply->arp_spa = if_in_addr(msg_out->iface);

    memcpy(reply->arp_tha, req->arp_sha, sizeof(reply->arp_tha));
    memcpy(reply->arp_tpa, req->arp_spa, sizeof(reply->arp_tpa));

    msg_out->len += sizeof(*reply);
}

static void arp_req_recv(struct msg *msg_in, struct msg *msg_out) {
    assert(msg_in != NULL);
    assert(msg_out != NULL);

    struct ether_arp *req = (struct ether_arp *)(msg_in->frame+msg_out->len);

    arp_cache_add(msg_in->iface,
            *(struct in_addr *)req->arp_spa,
            (struct ether_addr *)req->arp_sha);

    if (!in_addr_eq(*(struct in_addr *)req->arp_tpa,
                    if_in_addr(msg_out->iface)))
    {
        msg_out->status = MSG_DROPPED;
        return;
    }

    arp_reply_send(msg_in, msg_out);
}

static void arp_reply_recv(struct msg *msg_in, struct msg *msg_out) {
    assert(msg_in != NULL);
    assert(msg_out != NULL);

    struct ether_arp *reply = (struct ether_arp *)(msg_in->frame+msg_out->len);

    struct list_node *node
        = arp_cache_add(msg_in->iface,
                        *(struct in_addr *)reply->arp_spa,
                        (struct ether_addr *)reply->arp_sha);

    arp_cache_update = (struct arp_cache_e *)node->data;
}

void arp_handle(struct msg *msg_in, struct msg *msg_out) {
    assert(msg_in != NULL);
    assert(msg_out != NULL);

    struct ether_arp *arp_in = (struct ether_arp *)(msg_in->frame+msg_out->len);
    struct ether_arp *arp_out = (struct ether_arp *)(msg_out->frame+msg_out->len);

    if (ntohs(arp_in->ea_hdr.ar_hrd) != ARPHRD_ETHER
    ||  ntohs(arp_in->ea_hdr.ar_pro) != ETHERTYPE_IP)
    {
        msg_out->status = MSG_DROPPED;
        return;
    }

    switch (ntohs(arp_in->ea_hdr.ar_op)) {
    case ARPOP_REQUEST: {
        msg_out->iface = msg_in->iface;
        arp_req_recv(msg_in, msg_out);
        break;
    }

    case ARPOP_REPLY: {
        msg_out->status = MSG_DROPPED;
        arp_reply_recv(msg_in, msg_out);
        break;
    }
    }

    struct ethhdr *ethhdr_out = (struct ethhdr *)arp_out-1;

    memcpy(ethhdr_out->h_dest,
           arp_out->arp_tha,
           sizeof(ethhdr_out->h_dest));

    memcpy(ethhdr_out->h_source,
           arp_out->arp_sha,
           sizeof(ethhdr_out->h_source));

    ethhdr_out->h_proto = htons(ETHERTYPE_ARP);
}

struct list_node *arp_queue_add(struct rtable_e *rtable_e, struct msg *msg) {
    assert(msg != NULL);

    msg->status = MSG_QUEUED;

    struct arp_queue_e *e = malloc(sizeof(*e));
    e->rtable_e = rtable_e;
    memcpy(&e->msg, msg, sizeof(e->msg));

    return list_add_lst(arp_queue, e);
}

bool arp_queue_process(struct msg *msg) {
    assert(msg != NULL);

    if (arp_cache_update == NULL) {
        return false;
    }

    struct list_node *n;
    struct arp_queue_e *e;

    for (n = list_fst(arp_queue); n != arp_queue->back; n = n->next) {
        e = (struct arp_queue_e *)n->data;

        if (e->rtable_e->iface != arp_cache_update->iface) {
            continue;
        }

        if (!in_addr_eq(e->rtable_e->next_hop, arp_cache_update->in_addr)) {
            continue;
        }

        break;
    }

    if (n == arp_queue->back) {
        arp_cache_update = NULL;
        return false;
    }

    memcpy(msg, &e->msg, sizeof(*msg));

    free(list_rm(arp_queue, n));

    return true;
}

void arp_init(void) {
    for (size_t i = 0; i < IF_CNT; ++i) {
        arp_cache[i] = list_create();
        arp_cache_add(i, if_in_addr(i), if_ether_addr(i));
    }

    arp_cache_update = NULL;
    arp_queue = list_create();
}

void arp_term(void) {
    while (!list_empty(arp_queue)) {
        free(list_rm_fst(arp_queue));
    }

    list_destroy(arp_queue);

    for (size_t i = 0; i < IF_CNT; ++i) {
        while (!list_empty(arp_cache[i])) {
            free(list_rm_fst(arp_cache[i]));
        }

        list_destroy(arp_cache[i]);
    }
}
