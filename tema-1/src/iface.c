#include "iface.h"

#include "utils.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

static int if_sockfd[IF_CNT];

static int if_sock(const char *if_name) {
    int ret;

    int sockfd = socket(AF_PACKET, SOCK_RAW, 768);
    DIE(sockfd == -1, "could not open socket");

    struct ifreq ifreq;
    strcpy(ifreq.ifr_name, if_name);
    ret = ioctl(sockfd, SIOCGIFINDEX, &ifreq);
    DIE(ret, "ioctl SIOCGIFINDEX");

    struct sockaddr_ll addr;
    memset(&addr, 0x00, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifreq.ifr_ifindex;

    ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    DIE(ret == -1, "could not bind socket %d", sockfd);

    return sockfd;
}

static void sock_msg_send(int sockfd, struct msg *msg) {
    ssize_t nbytes = write(sockfd, msg->frame, msg->len);
    DIE(nbytes == -1, "could not write to socket %d", sockfd);
}

static void sock_msg_recv(int sockfd, struct msg *msg) {
    ssize_t nbytes = read(sockfd, msg->frame, MSG_FRAME_MAX_LEN);
    DIE(nbytes == -1, "could not read from socket %d", sockfd);

    msg->len = nbytes;
}

void if_msg_send(struct msg *msg) {
    sock_msg_send(if_sockfd[msg->iface], msg);
}

void if_msg_recv(struct msg *msg) {
    int ret;

    fd_set set;

    FD_ZERO(&set);
    while (true) {
        for (size_t i = 0; i < IF_CNT; ++i) {
            FD_SET(if_sockfd[i], &set);
        }

        ret = select(if_sockfd[IF_CNT-1]+1, &set, NULL, NULL, NULL);
        DIE(ret == -1, "select");

        for (size_t i = 0; i < IF_CNT; ++i) {
            if (FD_ISSET(if_sockfd[i], &set)) {
                sock_msg_recv(if_sockfd[i], msg);
                msg->iface = i;
                return;
            }
        }
    }

    DIE(true, "could not receive message");
}

struct ether_addr * if_ether_addr(size_t iface) {
    static struct ether_addr addr;

    struct ifreq ifreq;
    sprintf(ifreq.ifr_name, "r-%zu", iface);
    ioctl(if_sockfd[iface], SIOCGIFHWADDR, &ifreq);

    memcpy(addr.ether_addr_octet,
           ifreq.ifr_addr.sa_data,
           sizeof(addr.ether_addr_octet));

    return &addr;
}

struct in_addr if_in_addr(size_t iface) {
    struct ifreq ifreq;
    sprintf(ifreq.ifr_name, "r-%zu", iface);
    ioctl(if_sockfd[iface], SIOCGIFADDR, &ifreq);

    return ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr;
}

size_t if_from_ether_addr(struct ether_addr *addr) {
    for (size_t i = 0; i < IF_CNT; ++i) {
        if (memcmp(if_ether_addr(i), addr, sizeof(*addr)) == 0) {
            return i;
        }
    }

    return SIZE_MAX;
}

size_t if_from_in_addr(struct in_addr addr) {
    for (size_t i = 0; i < IF_CNT; ++i) {
        if (if_in_addr(i).s_addr == addr.s_addr) {
            return i;
        }
    }

    return SIZE_MAX;
}

void if_init(void) {
    int s0 = if_sock("r-0");
    int s1 = if_sock("r-1");
    int s2 = if_sock("r-2");
    int s3 = if_sock("r-3");

    if_sockfd[0] = s0;
    if_sockfd[1] = s1;
    if_sockfd[2] = s2;
    if_sockfd[3] = s3;
}

void if_term(void) {}
