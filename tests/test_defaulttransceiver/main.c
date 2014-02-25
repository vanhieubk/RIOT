/*
 * Copyright (C) 2014 Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Default application that shows a lot of functionality of RIOT
 *
 * @author      Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include "thread.h"
#include "msg.h"
#include "transceiver.h"

#define SND_BUFFER_SIZE     (100)
#define RCV_BUFFER_SIZE     (64)
#define RADIO_STACK_SIZE    (KERNEL_CONF_STACKSIZE_MAIN)

#define MAXVAL  (10)

char radio_stack_buffer[RADIO_STACK_SIZE];
msg_t msg_q[RCV_BUFFER_SIZE];

void radio_reply(radio_address_t sender, uint8_t payload)
{
    printf("sending reply..");

    msg_t m;
    transceiver_command_t t;
    radio_packet_t p;
    uint32_t response;

    p.data = (uint8_t *) &payload;
    p.length = 1;
    p.dst = sender;

    t.transceivers = TRANSCEIVER_DEFAULT;
    t.data = &p;

    m.type = SND_PKT;
    m.content.ptr = (char *) &t;
    msg_send_receive(&m, &m, transceiver_pid);
    response = m.content.value;

    printf("%"PRIi8".\n", (int8_t)response);
    return;
}

void radio(void)
{
    msg_t m;
    radio_packet_t *p;

    msg_init_queue(msg_q, RCV_BUFFER_SIZE);

    while (1) {
        msg_receive(&m);

        if (m.type == PKT_PENDING) {
            /* process radio packet */
            p = (radio_packet_t *) m.content.ptr;
            unsigned char c = p->data[0];
            radio_address_t sender = p->src;
            p->processing--;

            printf("Got radio message: %"PRIu8"\n", c);

            if (c >= MAXVAL) {
                puts("test finished");
            }

            c++;
            if (c <= MAXVAL) {
                radio_reply(sender, c);
            }
        }
    }
}

void init_transceiver(void)
{
    int radio_pid = thread_create(
                        radio_stack_buffer,
                        RADIO_STACK_SIZE,
                        PRIORITY_MAIN - 2,
                        CREATE_STACKTEST,
                        radio,
                        "radio");

    uint16_t transceivers = TRANSCEIVER_DEFAULT;

    transceiver_init(transceivers);
    (void) transceiver_start();
    transceiver_register(transceivers, radio_pid);
}

int main(void)
{
    init_transceiver();

    radio_reply(0, 0);

    return 0;
}
