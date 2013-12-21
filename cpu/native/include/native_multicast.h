int mcast_socket_outgoing(char *addr, char *group, char *port, char *ifname);

/**
 * opens an ipv6 socket, binds and joins with a given multicast group
 * group: multicast addr
 * port: udp port
 * ifidx: name of the interface to bind to
 */
int mcast_socket_incoming(char *group, char *port, char *ifname);
