int mcast_socket_outgoing(char *addr, char *group, char *port, int ifidx);

/**
 * opens an ipv6 socket, binds and joins with a given multicast group
 * group: multicast addr
 * port: udp port
 * ifidx: index of interface - 0 for default
 */
int mcast_socket_incoming(char *group, char *port, int ifidx);
