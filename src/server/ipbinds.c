#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "ipbinds.h"
#include "../hashset/ip_hashset.h"
#include "../hashset/fd_hashset.h"
#include "../queue/queue.h"

/*
typedef struct ipbinds {
	ip_hashset_ptr ips;
	ip_hashset_ptr timestamps;
	fd_hashset_ptr ports;
	pthread_mutex_t *hs_protect;
} ipbinds_t;
*/

ipbinds_t *new_ipbinds()
{
	ipbinds_t *ipbinds = NULL;
	ip_hashset_ptr ips = NULL;
	ip_hashset_ptr timestamps = NULL;
	fd_hashset_ptr ports = NULL;

	ipbinds = malloc(sizeof(ipbinds_t));

	if (!ipbinds) {
		perror("Memory error\n");
		return NULL;
	}
	fd_hashset_init_defaults(&ports);
	ip_hashset_init_defaults(&ips);
	ip_hashset_init_defaults(&timestamps);
	ipbinds->ips = ips;
	ipbinds->timestamps = timestamps;
	ipbinds->ports = ports;

	ipbinds->hs_protect = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(ipbinds->hs_protect, NULL);

	return ipbinds;
}

void free_ipbinds(ipbinds_t *ipbinds)
{
	if (!ipbinds) {
		return;
	}
	if (ipbinds->ips) {
		free_ip_hashset(ipbinds->ips);
		ipbinds->ips = NULL;
	}
	if(ipbinds->timestamps) {
		free_ip_hashset(ipbinds->timestamps);
		ipbinds->timestamps = NULL;
	}
	if (ipbinds->ports) {
		free_fd_hashset(ipbinds->ports);
		ipbinds->ports = NULL;
	}
	if (ipbinds->hs_protect) {
		pthread_mutex_destroy(ipbinds->hs_protect);
		free(ipbinds->hs_protect);
		ipbinds->hs_protect = NULL;
	}
	free(ipbinds);
}

/* remember to free this queue appropriately */
queue_t *ipbinds_get_ips(ipbinds_t *ipbinds)
{
	queue_t *q = NULL;

	pthread_mutex_lock(ipbinds->hs_protect);
	q = iphs_get_keys(ipbinds->ips);
	pthread_mutex_unlock(ipbinds->hs_protect);

	return q;
}

int ip_get_bound_port(ipbinds_t *ipbinds, unsigned char *ip)
{
	int port = ip_get_fd(ipbinds->ips, ip);
	int last_time, this_time;
	if (port) {
		this_time = (int)time(NULL);
		last_time = ip_get_time(ipbinds, ip);
		printf("%d %d\n", this_time, last_time);
		printf("time since last lookup: %d seconds\n", 
				this_time - last_time);
		ip_hashset_update(ipbinds->timestamps, ip, (int)time(NULL));
	}
	return port;
}

int ip_get_time(ipbinds_t *ipbinds, unsigned char *ip)
{
	return ip_get_fd(ipbinds->timestamps, ip);
}
unsigned char *port_get_bound_ip(ipbinds_t *ipbinds, int port)
{
	unsigned char *ip = fd_get_ip(ipbinds->ports, port);
	if (ip) {
		ip_hashset_update(ipbinds->timestamps, ip, (int)time(NULL));
	}
	return ip;
}

/* remember to free this queue appropriately */
/*
queue_t *get_ports(ipbinds_t *ipbinds)
{
	queue_t *q = NULL;

	pthread_mutex_lock(ipbinds->hs_protect);
	q = fdhs_get_keys(ipbinds->ports);
	pthread_mutex_unlock(ipbinds->hs_protect);

	return q;
}
*/

/*
void ipbinds_send_packet(ipbinds_t *ipbinds, packet_t *packet)
{
	int port = 0;

	pthread_mutex_lock(ipbinds->hs_protect);
	port = ip_get_fd(ipbinds->ips, packet->header.dst_ip);
	printf("%d was found in ipbinds.c\n", port);
	if (!port) {
		pthread_mutex_unlock(ipbinds->hs_protect);
		fprintf(stderr, "Failed to send message in ipbinds.c!!!\n");
		return;
	}

	send_packet(packet, port);

	pthread_mutex_unlock(ipbinds->hs_protect);

}
*/

void ipbinds_remove_port(ipbinds_t *ipbinds, int port)
{
	unsigned char *ip = NULL;
	pthread_mutex_lock(ipbinds->hs_protect);

	ip = fd_get_ip(ipbinds->ports, port);
	if (ip) {
		ip_hashset_remove(ipbinds->ips, ip);
	} else {
		fprintf(stderr, "this is weird when removing port\n");
		pthread_mutex_unlock(ipbinds->hs_protect);
		return;
	}
	fd_hashset_remove(ipbinds->ports, port);

	free(ip);
	pthread_mutex_unlock(ipbinds->hs_protect);
}

void ipbinds_remove_ip(ipbinds_t *ipbinds, unsigned char *ip)
{
	int port;

	pthread_mutex_lock(ipbinds->hs_protect);

	port = ip_get_fd(ipbinds->ips, ip);
	if (port) {
		fd_hashset_remove(ipbinds->ports, port);
	} else {
		fprintf(stderr, "this is weird when removing ip\n");
		pthread_mutex_unlock(ipbinds->hs_protect);
		return;
	}
	ip_hashset_remove(ipbinds->ips, ip);

	pthread_mutex_unlock(ipbinds->hs_protect);
}

int bind_ip_to_port(ipbinds_t *ipbinds, unsigned char *ip, int port)
{
	pthread_mutex_lock(ipbinds->hs_protect);

	if (!fd_hashset_insert(ipbinds->ports, port, ip)) {
		printf("failed to insert into port list");
		pthread_mutex_unlock(ipbinds->hs_protect);
		return 0;
	}
	if (!ip_hashset_insert(ipbinds->ips, ip, port)) {
		printf("failed to insert into ip list\n");
		pthread_mutex_unlock(ipbinds->hs_protect);
		ipbinds_remove_port(ipbinds, port);
		return 0;
	}
	if (!ip_hashset_insert(ipbinds->timestamps, ip, (int)time(NULL))) {
		printf("failed to insert into timestamps\n");
		pthread_mutex_unlock(ipbinds->hs_protect);
		ipbinds_remove_port(ipbinds, port);
		ipbinds_remove_ip(ipbinds, ip);
		return 0;
	}

	pthread_mutex_unlock(ipbinds->hs_protect);
	return 1;
}

/*
int login_connection(ipbinds_t *ipbinds, int port, unsigned char *ip)
{
	pthread_mutex_lock(ipbinds->hs_protect);

	if (!ip_hashset_insert(ipbinds->ips, ip, port)) {
		printf("failed to insert into ip list\n");
		pthread_mutex_unlock(ipbinds->hs_protect);
		return 0;
	}
	

	fd_hashset_update(ipbinds->ports, port, ip);


	pthread_mutex_unlock(ipbinds->hs_protect);
	return 1;
}
*/
