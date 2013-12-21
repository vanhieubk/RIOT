/**
 * zep.h implementation
 *
 * Copyright (C) 2013 Ludwig Ortmann
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup native_cpu
 * @{
 * @file
 * @author  Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#ifdef __MACH__
#define _POSIX_C_SOURCE
#include <net/if.h>
#undef _POSIX_C_SOURCE
#include <ifaddrs.h>
#include <net/if_dl.h>
#else
#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>
#endif


#define ENABLE_DEBUG    (1)
#include "debug.h"

#include "cpu.h"
#include "cpu-conf.h"
#include "zep.h"
#include "native_multicast.h"
#include "nativenet.h"
#include "nativenet_internal.h"
#include "native_internal.h"

int _native_zep_fd_out, _native_zep_fd_in;

int _native_marshall_zep(uint8_t *packbuf, radio_packet_t *packet);

void _native_handle_zep_input(void)
{
    DEBUG("_native_handle_zep_input\n");
#if 1
    int nread;
    union zep_frame zep;
    radio_packet_t p;

    DEBUG("_native_handle_zep_input\n");

    nread = real_read(_native_zep_fd_in, &zep, sizeof(union zep_frame));
    DEBUG("_native_handle_zep_input - read %d bytes\n", nread);
    if (nread > 0) {
        if (
                (zep.field.header.preamble[0] == ZEP_PREAMBLE[0])
                && (zep.field.header.preamble[1] == ZEP_PREAMBLE[1])
                && (zep.field.header.version == 1)
           ) {
                p.length = zep.field.header.length;
                p.src = ntohs(zep.field.header.device);
                uint16_t dst;
                memcpy(&dst, zep.field.header.reserved, 2);
                p.dst = ntohs(dst);
                p.rssi = 0;
                p.lqi = zep.field.header.lqi;
                p.processing = 0;
                p.data = zep.field.payload;
                DEBUG("_native_handle_zep_input: received packet of length %"PRIu16" for %"PRIu16" from %"PRIu16"\n", p.length, p.dst, p.src);
                _nativenet_handle_packet(&p);
        }
        else {
            DEBUG("ignoring non-zep frame\n");
        }

        /* work around lost signals */
        fd_set rfds;
        struct timeval t;
        memset(&t, 0, sizeof(t));
        FD_ZERO(&rfds);
        FD_SET(_native_zep_fd_in, &rfds);

        _native_in_syscall++; // no switching here
        if (select(_native_zep_fd_in +1, &rfds, NULL, NULL, &t) == 1) {
            int sig = SIGIO;
            real_write(_sig_pipefd[1], &sig, sizeof(int));
            _native_sigpend++;
            DEBUG("_native_handle_zep_input: sigpend++\n");
        }
        else {
            DEBUG("_native_handle_zep_input: no more pending zep data\n");
        }
        _native_in_syscall--;
    }
    else if (nread == -1) {
        if ((errno == EAGAIN ) || (errno == EWOULDBLOCK)) {
            //warn("read");
        }
        else {
            err(EXIT_FAILURE, "_native_handle_zep_input: read");
        }
    }
    else {
        errx(EXIT_FAILURE, "internal error _native_handle_zep_input");
    }
#endif
}

int _native_marshall_zep(uint8_t *framebuf, radio_packet_t *packet)
{
    DEBUG("_native_marshall_zep\n");
    int data_len;
    union zep_frame *f;
    f = (union zep_frame*)framebuf;
    radio_address_t dst;
    dst = htons(packet->dst);

    data_len = packet->length + sizeof(struct zep_header);
    if ((packet->length | ZEP_LENGTH_MASK)^ZEP_LENGTH_MASK) {
        warnx("packet too long, discarding");
        return 0;
    }

    f->field.header.preamble[0] = ZEP_PREAMBLE[0];
    f->field.header.preamble[1] = ZEP_PREAMBLE[1];
    f->field.header.version = 1;
    f->field.header.channel = _native_net_chan;
    f->field.header.device = htons(packet->src);
    f->field.header.lqi_mode = 0;
    f->field.header.lqi = 0;
    f->field.header.length = (uint8_t)packet->length;
    memcpy(f->field.header.reserved, &dst, 2);
    memcpy(f->field.payload, packet->data, packet->length);

    return data_len;
}

int8_t send_buf(radio_packet_t *packet)
{
    DEBUG("send_buf\n");
    uint8_t buf[sizeof(union zep_frame)];
    int nsent, to_send;

    memset(buf, 0, sizeof(buf));

    DEBUG("send_buf:  Converting packet of length %"PRIu16" from %"PRIu16" to %"PRIu16"\n", packet->length, packet->src, packet->dst);
    to_send = _native_marshall_zep(buf, packet);

    DEBUG("send_buf: trying to send %d bytes\n", to_send);

	struct sockaddr_in6 sa;
	memset(&sa, 0, sizeof(struct sockaddr_in6));
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(atoi(ZEP_DEFAULT_PORT));
	inet_pton(AF_INET6, "ff02::1", &sa.sin6_addr);

    if ((nsent = sendto(_native_zep_fd_out, buf, to_send, 0, (struct sockaddr*)&sa, sizeof(sa))) == -1) {
        warn("sendto");
        return -1;
    }

    return (nsent > INT8_MAX ? INT8_MAX : nsent);
}

int zep_init(char *node, char *ifname, char *service)
{
#if 1
    /* if this is a tap interface, make it usable */
    if(ifname != NULL) {
        int tap_fd;

#ifdef __MACH__ /* OSX */
        char clonedev[255] = "/dev/"; /* XXX bad size */
        strncpy(clonedev+5, ifname, 250);
#else /* Linux */
        struct ifreq ifr;
        char *clonedev = "/dev/net/tun";
#endif

        /* implicitly create the tap interface */
        if ((tap_fd = open(clonedev , O_RDWR)) == -1) {
            err(EXIT_FAILURE, "open(%s)", clonedev);
        }

#ifndef __MACH__ /* Linux */
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

        if (ioctl(tap_fd, TUNSETIFF, (void *)&ifr) == -1) {
            warn("ioctl");
            if (close(tap_fd) == -1) {
                warn("close");
            }
            exit(EXIT_FAILURE);
        }
#else
        printf("Please configure the IP address of %s to %s now and then press return\n", ifname, node);
        read(STDIN_FILENO, clonedev, 1);
#endif
    }
#endif

    /* set send callback */
    _nativenet_send_packet = send_buf;

    _native_zep_fd_in = mcast_socket_incoming("ff02::1", service, ifname);
    _native_zep_fd_out = mcast_socket_outgoing(node, "ff02::1", service, ifname);

    /* configure signal handler for fds */
    register_interrupt(SIGIO, _native_handle_zep_input);

    /* configure fds to send signals on io */
    if (fcntl(_native_zep_fd_in, F_SETOWN, getpid()) == -1) {
        err(EXIT_FAILURE, "zep_init(): fcntl(F_SETOWN)");
    }

    /* set file access mode to nonblocking */
    if (fcntl(_native_zep_fd_in, F_SETFL, O_NONBLOCK|O_ASYNC) == -1) {
        err(EXIT_FAILURE, "zep_init(): fcntl(F_SETFL)");
    }

    printf("RIOT native zep interface initialized.\n");
    return _native_zep_fd_in;
}
/** @} */
