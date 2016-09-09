#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <pthread.h>

#include "client_listener.h"
#include "client_speaker.h"
#include "../queue/queue.h"

/*** Macros **************************************************************/

#define TRUE			1
#define FALSE			0
#define DEFAULT_PORT	8002

/*** Struct Definition ***************************************************/

typedef struct client {
	pthread_t *listen_thread;
	client_speaker_t *speaker;
	client_listener_t *listener;
	int connected_status;
	pthread_mutex_t *connection_mutex;
	char *username;
	char *hostname;
	int hostport;
} chat_client_t;

/*** Function Headers ****************************************************/
/**
 * Create a new chat client datastructure.
 *
 * Assign heapspace and initialize everything to null;
 */
chat_client_t *new_client();

/** 
 * Free the chat client structure.
 *
 * @param[in] client: A pointer to the client structure to be free'd
 */
void free_chat_client(chat_client_t *client);

/**
 * Append the text in s to the ouput interface of the client.
 * This allows for easily expanding this implementation to a gui
 * interface, and allows easy interaction between the listener and
 * such a potential gui 
 *
 * @param[in] client:	The client structure on which a potential gui
 *						might be referenced.
 * @param[in] s:		The string to be displayed
 */
void client_append(chat_client_t *client, char *s);

/** 
 *	Receive a queue of usernames and display them to the standard
 *	io interface of the client.
 *
 *	@param[in] client:	The client structure of the currently running
 *						client
 *	@param[in] users_q: The names of the users to be displayed
 */
void client_show_online_users(chat_client_t *client, queue_t *users_q);

#endif
