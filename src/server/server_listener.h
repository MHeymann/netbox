#ifndef SERVER_LISTENER_H
#define SERVER_LISTENER_H

#include <pthread.h>
#include "server_speaker.h"
#include "users.h"

#define TRUE	1
#define FALSE	0

/*** Struct definitions **************************************************/

typedef struct listener {
	int *ports;
	int port_count;
	int run_status;
	pthread_mutex_t *status_lock;
	users_t *users;
	server_speaker_t *speaker;
} server_listener_t;

/*** Function Prototypes *************************************************/

/**
 * Allocate heap space for the server_listener_t data structure.
 * Set all fields to the corresponding values.
 *
 * @param[in] ports:		The ports on which to listen.
 * @param[in] port_count:	The number of ports in the previous argument.
 * @param[in] users:		A pointer to the users data structure.
 * @param[in] speaker:		A pointer to the speaker datastructure, 
 *							used by the speaker thread.
 *
 * @return A pointer to the newly allocated datastructure. NULL on 
 * failure.
 */
server_listener_t *new_server_listener(int *ports, int port_count, users_t *users, 
		server_speaker_t *speaker);

/**
 * Free the data structure allocated by new_server_listener.
 * 
 * @param[in] listener: The listener structure to be free'd.
 */
void server_listener_free(server_listener_t *listener);

/**
 * The default procedure to be run by the listener thread.
 *
 * @param[in] listener: The struct listener pointer to be used by the thread.
 *
 * @return NULL.
 */
void *listener_run(void *listener);

/**
 * Signal to the listening thread stop, causing it to break out of the 
 * while loop. 
 *
 * @param[in] listener: The listener datastructure being used by the 
 *						thread.
 */
void listener_stop(server_listener_t *listener);

/**
 * Check if the thread is marked as running.
 * 
 * @param[in] listener: The listener datastructure being used by the 
 *						thread.
 */
int listener_running(server_listener_t *listener);

#endif
