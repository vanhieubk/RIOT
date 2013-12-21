#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include "native_internal.h"
#include "native_multicast.h"
 
int mcast_socket_outgoing(char *addr, char *group, char *port, char *ifname)
{
	struct sockaddr_in6 sa;
	struct ipv6_mreq mreq;
    int s;
    int i;
    int ifidx;

	s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    i = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_outgoing: setsockopt(SO_REUSEADDR)");
	}

    if ((ifidx = if_nametoindex(ifname)) == 0) {
        err(EXIT_FAILURE, "mcast_socket_outgoing: if_nametoindex");
    }
    else {
        warn("mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_IF, %i", ifidx);
    }
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifidx, sizeof(ifidx)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_IF)");
	}

    i = 255;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &i, sizeof(i)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_HOPS)");
	}

    i = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &i, sizeof(i)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_LOOP)");
	}

    // TODO: bind to addr

	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(atoi(port));
	real_inet_pton(AF_INET6, group, &sa.sin6_addr);

	memcpy(&mreq.ipv6mr_multiaddr, &sa.sin6_addr, sizeof(mreq.ipv6mr_multiaddr));
	mreq.ipv6mr_interface = ifidx;

	if (setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *) &mreq, sizeof(mreq)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_outgoing: setsockopt(IPV6_JOIN_GROUP)");
	}

    return s;
}

int mcast_socket_incoming(char *group, char *port, char *ifname)
{
	struct sockaddr_in6 sa;
	struct ipv6_mreq mreq;

    int s; /* socket */
    int i; /* setsockopt option_value */
    int ifidx;

    /* create IPv6 UDP socket: */
	if ((s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_incoming: socket");
	}
 
    /* This is necessary to allow for multiple receivers at the same
     * group:port */
    i = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i))) {
		err(EXIT_FAILURE, "mcast_socket_incoming: setsockopt(SO_REUSEADDR)");
	}

    /* Linux man 7 ipv6:
     * Set the device for outgoing multicast packets on the socket.
     * This is allowed only  for SOCK_DGRAM  and  SOCK_RAW socket.
     * The argument is a pointer to an interface index (see
     * netdevice(7)) in an integer. */
    if ((ifidx = if_nametoindex(ifname)) == 0) {
        err(EXIT_FAILURE, "mcast_socket_incoming: if_nametoindex");
    }
    else {
        warn("mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_IF, %i", ifidx);
    }
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifidx, sizeof(ifidx))) {
		err(EXIT_FAILURE, "mcast_socket_incoming: setsockopt(IPV6_MULTICAST_IF)");
	}

    /* Linux man 7 ipv6:
     * Set  the  multicast hop limit for the socket.  Argument is a
     * pointer to an integer.  -1 in the value means use the route
     * default, otherwise it should be between 0 and 255.  */
    //i = 255;
    i = 255;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &i, sizeof(i))) {
		err(EXIT_FAILURE, "mcast_socket_incoming: setsockopt(IPV6_MULTICAST_HOPS)");
	}

    /* Linux man 7 ipv6:
     * Control whether the socket sees multicast packets that it has
     * send itself.  Argument is a pointer to boolean. */
    i = 0;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &i, sizeof(i))) {
		err(EXIT_FAILURE, "mcast_socket_incoming: setsockopt(IPV6_MULTICAST_LOOP)");
	}
 
    char any[200];
    if (real_inet_ntop(AF_INET6, &in6addr_any, any, sizeof(any)) == NULL) {
        warn("mcast_socket_incoming: real_inet_ntop");
    }

    printf("bound_socket: attempting to bind to %s port %s\n", any, port);
    /* Initalize sa: */
	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(atoi(port));
    sa.sin6_addr = in6addr_any;
 
	if (bind(s, (struct sockaddr *) &sa, sizeof(sa))) {
		err(EXIT_FAILURE, "bound_socket: bind");
	}
 
    printf("mcast_socket_incoming: attempting to join %s\n", group);
	memset(&sa, 0, sizeof(sa));
	i = real_inet_pton(AF_INET6, group, &sa.sin6_addr);
	switch (i) {
        case -1:
            err(EXIT_FAILURE, "mcast_socket_incoming, real_inet_pton(%s)", group);
            break;
        case 0:
            errx(EXIT_FAILURE, "mcast_socket_incoming, real_inet_pton(%s): not a valid address", group);
            break;
        default:
            break;
    }

	memcpy(&mreq.ipv6mr_multiaddr, &sa.sin6_addr, sizeof(mreq.ipv6mr_multiaddr));
	mreq.ipv6mr_interface = ifidx;

	if (setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *) &mreq, sizeof(mreq))) {
		err(EXIT_FAILURE, "mcast_socket_incoming: setsockopt(IPV6_JOIN_GROUP)");
	}

    return s;
}

#if 0
int sock_loop(int sock)
{
	char buf[1400];
	fd_set fds;
    int running = 1;

	while (running == 1) {

        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 0;

        int rc = select(sock + 1, &fds, NULL, NULL, NULL);
        if (rc == -1) {
            err(EXIT_FAILURE, "sock_loop: select");
            break;
        }
        else if (rc == 0) {
            continue;
        }

		int len = read(sock, buf, 1400);
		buf[len] = '\0';

		if (len > 0) {
			len = printf("sock_loop: read: %s", buf);
		} else if (len == -1) {
            err(EXIT_FAILURE, "sock_loop: read");
		} else {
            warnx("sock_loop: socket closed");
            running = 0;
		}
	}
    return 0;
}

int main(int argc, char **argv)
{
    int s;

    if (argc < 4) {
        printf("Usage: %s <address> <group> <port>\n", argv[0]);
        printf("\n\
\taddress:\tIPv6 address to listen on\n\
\tport:\t\tport to listen on\n\
\tgroup:\t\tIPv6 multicast group to join\n");
        printf("\n\
Example:\n\t%s ff02::5:6 ff02::5:6 12345\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    if ((s = mcast_socket_incoming(argv[2], argv[3], 0)) == -1) {
        errx(EXIT_FAILURE, "main: mcast_socket_incoming failed");
    }

    sock_loop(s);

    return EXIT_SUCCESS;
}
#endif
