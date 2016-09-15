#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h> /* write */
#include <errno.h>
#include <pthread.h>


#include "client_listener.h"
#include "client_speaker.h"
#include "chat_client.h"
#include "../packet/packet.h"
#include "../packet/code.h"

#define TRUE		1
#define FALSE		0

/*
typedef struct client_listener {
	int sd;
	void *chat_client;
	char *name;
	int running;
	pthread_mutex_t *listen_mutex;
} client_listener_t;
*/

/*** Helper Function Prototypes ******************************************/

void listener_go(client_listener_t *listener);
char *listen_strdup(char *s);
unsigned char *listen_ipdup(unsigned char *s);
int listener_is_running(client_listener_t *listener);
int listen_ipcmp(unsigned char *a, unsigned char *b);

/*** Functions ***********************************************************/

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
client_listener_t *new_client_listener(int sd, void *client, unsigned char *client_ip)
{
	client_listener_t *listener = NULL;

	listener = malloc(sizeof(client_listener_t));
	if (!listener) {
		fprintf(stderr, "memory error\n");
	} else {
		listener->sd = sd;
		listener->chat_client = client;
		listener->client_ip = listen_ipdup(client_ip);
		listener->running = TRUE;
		listener->listen_mutex = malloc(sizeof(client_listener_t));
		if (listener->listen_mutex) {
			pthread_mutex_init(listener->listen_mutex, NULL);
		} else {
			free_client_listener(listener);
			listener = NULL;
		}
	}

	return listener;
}

/**
 * Free all the fields in the struct client_listener as needed.
 * 
 * @param[in] listener:	A pointer to the listener to free.
 */
void free_client_listener(client_listener_t *listener)
{
	if (!listener) {
		return;
	}
	if (listener->sd) {
		/*
		close(listener->sd);
		*/
		listener->sd = 0;
	} 
	listener->chat_client = NULL;
	if (listener->client_ip) {
		free(listener->client_ip);
		listener->client_ip = NULL;
	}
	listener->running = TRUE;
	if (listener->listen_mutex) {
		pthread_mutex_destroy(listener->listen_mutex);
		free(listener->listen_mutex);
		listener->listen_mutex = NULL;
	}
	free(listener);
}

/**
 * The method to be run when the listen thread is started.
 * @param[in] listener: A pointer to the listener keeping info for the thread.
 *
 * @return Null in all cases.
 */
void *run_client_listener(void *listener)
{
	if (!listener) {
		printf("Invalid pointer provided for running listener\n");
	} else {
		listener_go((client_listener_t *)listener);
	}
	return NULL;

}

/**
 * Signal to the listener thread that it must stop. 
 *
 * @param[in] listener: A pointer to the listener datastructure of the thread
 * that must be stopped.
 */
void stop_listener(client_listener_t *listener)
{
	pthread_mutex_lock(listener->listen_mutex);
	listener->running = FALSE;
	pthread_mutex_unlock(listener->listen_mutex);
}

/*** Helper Functions ****************************************************/

void listener_go(client_listener_t *listener)
{
	int activity;
	fd_set readfds;
	struct timeval tv;
	packet_t *packet = NULL;
	char s[1024];
	printf("client listener starting up\n");

	/* see if the running flag is still set as true */
	while (listener_is_running(listener)) {
		/* clear the listen set */
		FD_ZERO(&readfds);	

		/* add the sd to the set for listening */
		FD_SET(listener->sd, &readfds);

		/* Set the delay period.  This must be reset every time,
		 * as this value gets updated by the system as the time
		 * is counted off */
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		/* wait for incoming traffic */
		activity = select(listener->sd + 1, &readfds, NULL, NULL, &tv);

		/* check for errors */
		if ((activity < 0) && (errno != EINTR)) {
			printf("Select error\n");
		}
		
		/* if activity is 0, go back to while condition and see if we're
		 * still running */
		if (activity == 0) {
			continue;
		}
		
		/* Quick sanity test */
		if (!FD_ISSET(listener->sd, &readfds)) {
			printf("This is very weird, only one sd was set\n");
			printf("but now after select it is not marked as set\n");
			continue;
		}

		/* receive the incoming data as a packet */
		packet = NULL;
		packet = receive_packet(listener->sd);
		/* handle appropriately, based on code in packet */
		if (!packet) {
			printf("Server went offline\n");
			disconnect_client(listener->chat_client);
			break;
		} else if(packet->code == SEND) {
			sprintf(s, "%d.%d.%d.%d:%d:: %s\n", 
					(int)packet->header.src_ip[0], 
					(int)packet->header.src_ip[1], 
					(int)packet->header.src_ip[2], 
					(int)packet->header.src_ip[3],
					packet->header.src_port,
					(char *)packet->data);
			client_append((chat_client_t *)listener->chat_client, s);
		} else if(packet->code == ECHO) {
			sprintf(s, "YOU echoed: %s\n", packet->data);
			client_append((chat_client_t *)listener->chat_client, s);
		} else if(packet->code == BROADCAST) {
			if (listen_ipcmp(listener->client_ip, packet->header.src_ip) == 0) {
				sprintf(s, "YOU Broadcast: %s\n", packet->data);
			} else {
				sprintf(s, "%d.%d.%d.%d Broadcast: %s\n",
						(int)packet->header.src_ip[0],
						(int)packet->header.src_ip[1],
						(int)packet->header.src_ip[2],
						(int)packet->header.src_ip[3], packet->data);
			}
			client_append((chat_client_t *)listener->chat_client, s);
		} else if(packet->code == GET_ULIST) {
			printf("showing users\n");
			client_show_online_users((chat_client_t *)listener->chat_client, packet->users);
		} else {
			printf("The server did something unorthodox\n");
		}
		if (packet) {
			free_packet(packet);
			packet = NULL;
		}
	}
}

/* strdup is not ansi c, hence defined explicitly here */
char *listen_strdup(char *s)
{
	char *c = malloc(strlen(s) + 1);
	int i = 0, j = 0;

	while ((c[i++] = s[j++]));

	return c;
}

int listen_ipcmp(unsigned char *a, unsigned char *b)
{
	int ret_val;
	if (a[0] == b[0]) {
		if (a[1] == b[1]) {
			if (a[2] == b[2]) {
				if (a[3] == b[3]) {
					ret_val = 0;
				} else {
					ret_val = (int)a[3] - (int)b[3];
				}
			} else {
				ret_val = (int)a[2] - (int)b[2];
			}
		} else {
			ret_val = (int)a[1] - (int)b[1];
		}
	} else {
		ret_val = (int)a[0] - (int)b[0];
	}
	return ret_val;
}

/* check the value of the running flag, keeping in mind this might be a race
 * condition */
int listener_is_running(client_listener_t *listener) 
{
	int ret_val;

	pthread_mutex_lock(listener->listen_mutex);
	ret_val = listener->running;
	pthread_mutex_unlock(listener->listen_mutex);
	
	return ret_val;
}

unsigned char *listen_ipdup(unsigned char *s) 
{
	int i;
	unsigned char *c = malloc(4);

	for (i = 0; i < 4; i++) {
		c[i] = s[i];
	}

	return c;
}


