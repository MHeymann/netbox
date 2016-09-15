#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "users.h"
#include "../hashset/ip_hashset.h"
#include "../hashset/fd_hashset.h"
#include "../queue/queue.h"

/*
typedef struct users {
	ip_hashset_ptr ips;
	fd_hashset_ptr sockets;
	pthread_mutex_t *hs_protect;
} users_t;
*/

users_t *new_users()
{
	users_t *users = NULL;
	ip_hashset_ptr ips = NULL;
	fd_hashset_ptr sockets = NULL;

	users = malloc(sizeof(users_t));

	if (!users) {
		perror("Memory error\n");
		return NULL;
	}
	fd_hashset_init_defaults(&sockets);
	ip_hashset_init_defaults(&ips);
	users->ips = ips;
	users->sockets = sockets;

	users->hs_protect = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(users->hs_protect, NULL);

	return users;
}

void free_users(users_t *users)
{
	if (!users) {
		return;
	}
	if (users->ips) {
		free_ip_hashset(users->ips);
		users->ips = NULL;
	}
	if (users->sockets) {
		free_fd_hashset(users->sockets);
		users->sockets = NULL;
	}
	if (users->hs_protect) {
		pthread_mutex_destroy(users->hs_protect);
		free(users->hs_protect);
		users->hs_protect = NULL;
	}
	free(users);
}

/* remember to free this queue appropriately */
queue_t *get_ips(users_t *users)
{
	queue_t *q = NULL;

	pthread_mutex_lock(users->hs_protect);
	q = iphs_get_keys(users->ips);
	pthread_mutex_unlock(users->hs_protect);

	return q;
}

/* remember to free this queue appropriately */
queue_t *get_fds(users_t *users)
{
	queue_t *q = NULL;

	pthread_mutex_lock(users->hs_protect);
	q = fdhs_get_keys(users->sockets);
	pthread_mutex_unlock(users->hs_protect);

	return q;
}



void users_send_packet(users_t *users, packet_t *packet)
{
	int fd = 0;

	pthread_mutex_lock(users->hs_protect);
	fd = ip_get_fd(users->ips, packet->header.dst_ip);
	if (!fd) {
		pthread_mutex_unlock(users->hs_protect);
		fprintf(stderr, "Failed to send message in users.c!!!\n");
		return;
	}

	send_packet(packet, fd);

	pthread_mutex_unlock(users->hs_protect);

}

void remove_channel(users_t *users, int fd)
{
	unsigned char *ip = NULL;
	pthread_mutex_lock(users->hs_protect);

	ip = fd_get_ip(users->sockets, fd);
	if (ip) {
		ip_hashset_remove(users->ips, ip);
	} else {
		fprintf(stderr, "this is weird when removing fd\n");
		pthread_mutex_unlock(users->hs_protect);
		return;
	}
	fd_hashset_remove(users->sockets, fd);

	printf("User %d.%d.%d.%d went offline, %d still online\n", 
			(int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3],
			ip_hashset_content_count(users->ips));

	free(ip);
	pthread_mutex_unlock(users->hs_protect);
}

void remove_ip(users_t *users, unsigned char *ip)
{
	int fd;

	pthread_mutex_lock(users->hs_protect);

	fd = ip_get_fd(users->ips, ip);
	if (fd) {
		fd_hashset_remove(users->sockets, fd);
	} else {
		fprintf(stderr, "this is weird when removing ip\n");
		pthread_mutex_unlock(users->hs_protect);
		return;
	}
	ip_hashset_remove(users->ips, ip);

	printf("User %d.%d.%d.%d went offline, %d still online\n", 
			(int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3],
			ip_hashset_content_count(users->ips));

	pthread_mutex_unlock(users->hs_protect);
}

int add_connection(users_t *users,int fd)
{
	unsigned char localhost[4] = {127,
	0,
	0,
	1
	};
	pthread_mutex_lock(users->hs_protect);

	if (!fd_hashset_insert(users->sockets, fd, localhost)) {
		printf("failed to insert into socket list");
		return 0;
	}

	pthread_mutex_unlock(users->hs_protect);
	return 1;
}

int login_connection(users_t *users, int fd, unsigned char *ip)
{
	pthread_mutex_lock(users->hs_protect);

	if (!ip_hashset_insert(users->ips, ip, fd)) {
		printf("failed to insert into ip list\n");
		pthread_mutex_unlock(users->hs_protect);
		return 0;
	}
	

	fd_hashset_update(users->sockets, fd, ip);


	pthread_mutex_unlock(users->hs_protect);
	return 1;
}

