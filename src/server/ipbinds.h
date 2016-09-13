#ifndef IPBINDS_H
#define IPBINDS_H

#include <stdlib.h>
#include <pthread.h>

#include "../hashset/ip_hashset.h"
#include "../hashset/fd_hashset.h"
#include "../queue/queue.h"
/*
#include "../packet/packet.h"
*/

typedef struct ipbinds {
	ip_hashset_ptr ips;
	ip_hashset_ptr timestamps;
	fd_hashset_ptr ports;
	pthread_mutex_t *hs_protect;
} ipbinds_t;

/**
 * Allocate heap space for the ipbinds_t struct.
 */
ipbinds_t *new_ipbinds();

/**
 * Free a ipbinds_t struct heap space.
 *
 * @param[in] ipbinds: The struct to be free'd.
 */
void free_ipbinds(ipbinds_t *ipbinds);

/**
 * Get a queue of the userips of all online ipbinds.
 *
 * @param[in] ipbinds: The struct maintaining a list of online ipbinds.
 *
 * @return A queue of all the currently online userips.
 */
queue_t *ipbinds_get_ips(ipbinds_t *ipbinds);

int ip_get_bound_port(ipbinds_t *ipbinds, unsigned char *ip);
int ip_get_time(ipbinds_t *ipbinds, unsigned char *ip);
unsigned char *port_get_bound_ip(ipbinds_t *ipbinds, int port);

/**
 * Get the socket file descriptors of all online ipbinds.
 */
/*
queue_t *get_ports(ipbinds_t *ipbinds);
*/

/**
 * Send a packet.
 */
/*
void ipbinds_send_packet(ipbinds_t *ipbinds, packet_t *packet);
*/

/**
 * Remove a file descriptor from ipbinds.
 */
void ipbinds_remove_port(ipbinds_t *ipbinds, int port);

/**
 * Remove a name from ipbinds.
 */
void ipbinds_remove_ip(ipbinds_t *ipbinds, unsigned char *ip);

/**
 * Add a new socket file descriptor to the ipbinds.
 */
int bind_ip_to_port(ipbinds_t *ipbinds, unsigned char *ip, int port);

/**
 * process the overhead of a newly logged in user.
 */
/*
int login_connection(ipbinds_t *ipbinds, int port, unsigned char *ip);
*/

#endif
