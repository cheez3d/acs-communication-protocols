#include "arp.h"
#include "ether.h"
#include "iface.h"
#include "rtable.h"
#include "utils.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    if_init();
    arp_init();
    rtable_init();

    while (true) {
        struct msg msg_in;
        memset(&msg_in, 0x00, sizeof(msg_in));
        msg_in.status = MSG_OK;
        msg_in.len = 0;

        struct msg msg_out;
        memset(&msg_out, 0x00, sizeof(msg_out));
        msg_out.status = MSG_OK;
        msg_out.len = 0;

        if (!arp_queue_process(&msg_in)) {
            if_msg_recv(&msg_in);
        }

        ether_handle(&msg_in, &msg_out);

        if (msg_out.status == MSG_DROPPED) {
            continue;
        }

        if_msg_send(&msg_out);
    }

    rtable_term();
    arp_term();
    if_term();
}
