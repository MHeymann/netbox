#ifndef SERVER_SPEAKER_H
#define SERVER_SPEAKER_H

#include <semaphore.h>
#include "../queue/queue.h"
#include "../packet/packet.h"
#include "ipbinds.h"
#include "users.h"

#define TRUE	1
#define FALSE	0

typedef struct speaker {
	users_t *users;
	sem_t *queue_sem;
	pthread_mutex_t *queue_lock;
	queue_t *q;
	int run_status;
	pthread_mutex_t *status_lock;
	ipbinds_t *iptable;
	int ip_timeout;
	unsigned char serv_ip[4];
} server_speaker_t;

/**
 * Allocate heap space for the struct.
 *
 * @param[in] users: The users currently online.
 *
 * @return The new data structure.
 */
server_speaker_t *new_server_speaker(users_t *users, unsigned char *serv_ip);

/**
 * Free the struct.
 *
 * @param[in] speaker: the struct to be free'd.
 */
void server_speaker_free(server_speaker_t *speaker);

/**
 * Add a packet to the internal queue to be processed by the speaker.
 *
 * @param[in] speaker:	The speaker thread's data structure that contains
 *						the queue.
 * @param[in] packet:	The packet datastructure that must be processed.
 */
void add_packet_to_queue(server_speaker_t *speaker, packet_t *packet);

/** 
 * Send a list of online users to all online users.
 *
 * @param[in] speaker:	The speaker used by this thread.
 */
void push_user_list(server_speaker_t *speaker); 

/**
 * Send a packet to all online users.
 *
 * @param[in] speaker:	The speaker sending packets out.
 * @param[in] packet:	The packet to be broadcast.
 */
void broadcast(server_speaker_t *speaker, packet_t *packet);

/**
 * The process to be run when creating the thread.
 *
 * @param[in] speaker:	The struct to be used for overhead by the
 *						speaker thread.
 */
void *speaker_run(void *speaker);

/**
 * Signal to the speaker thread to stop, breaking out of it's while loop.
 * 
 * @param[in] speaker: The struct to administer overhead.
 */
void speaker_stop(server_speaker_t *speaker);

/**
 * Check whether the speaker thread is currently marked as running.
 *
 * @param[in] speaker: The struct to administer overhead.
 */
int speaker_running(server_speaker_t *speaker);

void refresh_ip_binds(server_speaker_t *speaker);

#endif
