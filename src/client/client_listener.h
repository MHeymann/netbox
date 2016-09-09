#ifndef CLIENT_LISTENER_H
#define CLIENT_LISTENER_H

#define MAX_LINE 1024

/*** Struct Definitions **************************************************/

/**
 *  A struct that contains all necessary info of a listener
 */
typedef struct client_listener {
	int sd;							/* A socket discriptor */
	void *chat_client;				/* A pointer to the client structure */
	char *name;						/* The username of the client */
	int running;					/* Integer that functions as boolean */
	pthread_mutex_t *listen_mutex;	/* A mutex for protecting the running boolean */
} client_listener_t;

/*** Function Prototypes *************************************************/

/**
 *	Allocate memory for a new listener thread to use.
 *	All variales are initialized and zeroed.
 *
 *	@param[in] sd:		The file descriptor of the socket used.
 *	@param[in] client:	The chat_client_t instance that this belongs to.
 *	@param[in] name:	The username of the client.
 *
 *	@return A pointer to the memory location allocated for the listener.
 */
client_listener_t *new_client_listener(int sd, void *client, char *name);

/**
 * Free all the fields in the struct client_listener as needed.
 * 
 * @param[in] listener:	A pointer to the listener to free.
 */
void free_client_listener(client_listener_t *listener);

/**
 * The method to be run when the listen thread is started.
 * @param[in] listener: A pointer to the listener keeping info for the thread.
 *
 * @return Null in all cases.
 */
void *run_client_listener(void *listener);

/**
 * Signal to the listener thread that it must stop. 
 *
 * @param[in] listener: A pointer to the listener datastructure of the thread
 * that must be stopped.
 */
void stop_listener(client_listener_t *listener);

#endif
