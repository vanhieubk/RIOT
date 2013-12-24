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
        warnx("mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_IF, %i)", ifidx);
    }
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifidx, sizeof(ifidx)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_IF)");
	}

    i = 255;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &i, sizeof(i)) == -1) {
		err(EXIT_FAILURE, "mcast_socket_outgoing: setsockopt(IPV6_MULTICAST_HOPS)");
	}

    i = 0;
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
        warnx("mcast_socket_incoming: setsockopt(IPV6_MULTICAST_IF, %i)", ifidx);
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

    printf("mcast_socket_incoming: attempting to bind to %s port %s\n", any, port);
    /* Initalize sa: */
	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(atoi(port));
    sa.sin6_addr = in6addr_any;
 
	if (bind(s, (struct sockaddr *) &sa, sizeof(sa))) {
		err(EXIT_FAILURE, "mcast_socket_incoming: bind");
	}
 
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

    printf("mcast_socket_incoming: attempting to join %s\n", group);
	if (setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *) &mreq, sizeof(mreq))) {
		err(EXIT_FAILURE, "mcast_socket_incoming: setsockopt(IPV6_JOIN_GROUP)");
	}

    return s;
}
