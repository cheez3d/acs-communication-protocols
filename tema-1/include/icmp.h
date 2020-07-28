#ifndef ICMP_H
#define ICMP_H

#include "iface.h"

#include <stdint.h>

void icmp_send(struct msg *msg_in, struct msg *msg_out, uint8_t type);

void icmp_handle(struct msg *msg_in, struct msg *msg_out);

#endif // ICMP_H
