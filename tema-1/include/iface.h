#ifndef IFACE_H
#define IFACE_H

#include <netinet/if_ether.h>
#include <sys/types.h>

#define IF_CNT 4

#define MSG_FRAME_MAX_LEN ETH_FRAME_LEN + ETH_FCS_LEN

enum msg_status {
    MSG_OK,
    MSG_DROPPED,
    MSG_QUEUED,
};

struct msg {
    enum msg_status status;
    size_t iface;
    char frame[MSG_FRAME_MAX_LEN];
    size_t len;
};

void if_msg_send(struct msg *msg);
void if_msg_recv(struct msg *msg);

struct ether_addr * if_ether_addr(size_t iface);
struct in_addr if_in_addr(size_t iface);

size_t if_from_ether_addr(struct ether_addr *addr);
size_t if_from_in_addr(struct in_addr addr);

void if_init(void);
void if_term(void);

#endif // IFACE_H
