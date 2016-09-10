#ifndef USERS_H
#define USERS_H

#include <stdlib.h>
#include <pthread.h>

#include "../hashset/string_hashset.h"
#include "../hashset/fd_hashset.h"
#include "../queue/queue.h"
#include "../packet/packet.h"

typedef struct users {
	string_hashset_ptr names;
	fd_hashset_ptr sockets;
	pthread_mutex_t *hs_protect;
} users_t;

/**
 * Allocate heap space for the users_t struct.
 */
users_t *new_users();

/**
 * Free a users_t struct heap space.
 *
 * @param[in] users: The struct to be free'd.
 */
void free_users(users_t *users);

/**
 * Get a queue of the usernames of all online users.
 *
 * @param[in] users: The struct maintaining a list of online users.
 *
 * @return A queue of all the currently online usernames.
 */
queue_t *get_names(users_t *users);

/**
 * Get the socket file descriptors of all online users.
 */
queue_t *get_fds(users_t *users);

/**
 * Send a packet.
 */
void users_send_packet(users_t *users, packet_t *packet);

/**
 * Remove a file descriptor from users.
 */
void remove_channel(users_t *users, int fd);

/**
 * Remove a name from users.
 */
void remove_name(users_t *users, char *name);

/**
 * Add a new socket file descriptor to the users.
 */
int add_connection(users_t *users, int fd);

/**
 * process the overhead of a newly logged in user.
 */
int login_connection(users_t *users, int fd, char *name);

#endif
