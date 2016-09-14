#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> /* inet_addr */
#include <unistd.h> /* write */
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#include "server_listener.h"
#include "users.h"
#include "ipbinds.h"
#include "../packet/code.h"
#include "../hashset/fd_hashset.h"
#include "../hashset/ip_hashset.h"

/*** Macros **************************************************************/

#define BUFFER_SIZE		2048
#define DEFAULT_PORT	8008
#define TRUE			1
#define FALSE			0
#define MAX_CLIENTS		(int) sizeof(long)

	

/*** Helper Function Prototypes ******************************************/

void listener_go(server_listener_t *listener);
int check_user_password(unsigned char *name, char *pw);
char *listen_strdup(char *s);
unsigned char *listen_ipdup(unsigned char *s);

unsigned char null_address[4] = {
	'\0',
	'\0',
	'\0',
	'\0'
};


/*** Functions ***********************************************************/

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
		server_speaker_t *speaker)
{
	int i = 0;
	server_listener_t *listener = NULL;
	int lports[1];

	lports[0] = 8002;

	if (!users) {
		fprintf(stderr, "Error: no users_t * provided\n");
		return NULL;
	}
	if (!speaker) {
		fprintf(stderr, "Error: no speaker provided\n");
		return NULL;
	}

	if (!ports) {
		ports = (int *)lports;
		printf("Listener defaulting to port 8002\n");
		port_count = 1;
	}

	listener = malloc(sizeof(server_listener_t));
	if (!listener) {
		fprintf(stderr, "error mallocing listener\n");
		return NULL;
	}
	listener->ports = malloc(sizeof(int) * port_count);
	for (i = 0; i < port_count; i++) {
		listener->ports[i] = ports[i];
	}
	listener->port_count = port_count;
	listener->run_status = TRUE;
	listener->status_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(listener->status_lock, NULL);
	listener->users = users;
	listener->speaker = speaker;

	listener->ip_allocator = new_address_allocator();
	listener->mac_allocator = new_mac_list();

	return listener;
}

/**
 * Free the data structure allocated by new_server_listener.
 * 
 * @param[in] listener: The listener structure to be free'd.
 */
void server_listener_free(server_listener_t *listener)
{
	if (!listener) {
		return;
	}
	if (listener->ports) {
		free(listener->ports);
		listener->ports = NULL;
	}
	if (listener->status_lock) {
		pthread_mutex_destroy(listener->status_lock);
		free(listener->status_lock);
		listener->status_lock = NULL;
	}
	if (listener->users) {
		/* this gets free'd in main */
		/*
		free_users(listener->users);
		*/
		listener->users = NULL;
	}
	if (listener->speaker) {
		/* this gets free'd in main */
		listener->speaker = NULL;
	}

	if (listener->ip_allocator) {
		free_address_allocator(listener->ip_allocator);
		listener->ip_allocator = NULL;
	}

	if (listener->mac_allocator) {
		free_mac_list(listener->mac_allocator);
		listener->mac_allocator = NULL;
	}

	free(listener);
}

/**
 * The default procedure to be run by the listener thread.
 *
 * @param[in] listener: The struct listener pointer to be used by the thread.
 *
 * @return NULL.
 */
void *listener_run(void *listener) 
{
	if (!listener) {
	} else {
		listener_go((server_listener_t *)listener);
	}
	return NULL;
}

/**
 * Signal to the listening thread stop, causing it to break out of the 
 * while loop. 
 *
 * @param[in] listener: The listener datastructure being used by the 
 *						thread.
 */
void listener_stop(server_listener_t *listener)
{
	pthread_mutex_lock(listener->status_lock);
	listener->run_status = FALSE;
	pthread_mutex_unlock(listener->status_lock);
}

/**
 * Check if the thread is marked as running.
 * 
 * @param[in] listener: The listener datastructure being used by the 
 *						thread.
 */
int listener_running(server_listener_t *listener)
{
	int status;
	pthread_mutex_lock(listener->status_lock);
	status = listener->run_status;
	pthread_mutex_unlock(listener->status_lock);
	return status;
}

/*** Helper Functions ****************************************************/

/* the actual workhorse function */
void listener_go(server_listener_t *listener)
{
	int master_socket = -1, new_socket = -1, i = 0;
	int sd = 0;
	int opt = TRUE;
	int addrlen = -1;
	int activity;
	int max_sd;
	struct sockaddr_in address;
	fd_set readfds;
	int port_count;
	queue_t *online_list2 = NULL;
	node_t *n = NULL;
	struct timeval tv;
	unsigned char *ip_add = NULL;
	unsigned char *mac_add = NULL;

	packet_t *packet = NULL;
	packet_t *p = NULL;

	port_count = listener->port_count;

	/* bind to the different ports for listening */
	for (i = 0; i < port_count; i++) {
	/* Create master sockets */
		if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
			perror("socket failed\n");
			exit(EXIT_FAILURE);
		}
		printf("Port %d selected\n", listener->ports[i]);
		/* good habit, not necessary: set master socket to allow multiple
		 * connections/
		 */
		if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
			perror("setsockopt\n");
			exit(EXIT_FAILURE);
		}
	
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		address.sin_port = htons(listener->ports[i]);
	
		/* Bind */
		if (bind(master_socket, (struct sockaddr *)&address, 
					sizeof(address)) < 0) {
			perror("bind failed\n");
			exit(EXIT_FAILURE);
		}
		printf("Bind to %d done\n", listener->ports[i]);
	
		/* listen */
		if (listen(master_socket, 3) < 0) {
			perror("listen\n");
			exit(EXIT_FAILURE);
		}

		listener->ports[i] = master_socket;
	}


	/* Accept incoming connections */
	printf("Waiting for incoming connections...\n");
	addrlen = sizeof(address);
	while (listener_running(listener)) {
		/* clear the socket set */
		FD_ZERO(&readfds);

		max_sd = 0;
		for (i = 0; i < port_count; i++) {
		/* add masters for listening */
			FD_SET(listener->ports[i], &readfds);
			if (max_sd < listener->ports[i]) {
				max_sd = listener->ports[i];
			}
		}

		/* all sockets currently opened added to set */
		online_list2 = get_fds(listener->users);
		for (n = online_list2->head; n; n = n->next) {
			sd = (int)((long)n->data);
			if (sd <= 0) {
				printf("this shouldn't be possible when translating a name to an sd.\n");
				continue;
			}
			FD_SET(sd, &readfds);

			if (max_sd < sd) {
				max_sd = sd;
			}
		}
		free_queue(online_list2);
		online_list2 = NULL;

		/* ret	 = select(nfds, readfds, writefds, exceptfds, timeout); */
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

		if ((activity < 0) && (errno != EINTR)) {
			printf("select error\n");
		}
		if (activity == 0) {
			continue;
		}

		online_list2 = get_fds(listener->users);
		
		for (i = 0; i < port_count; i++) {
			/* master socket, => incoming connection */
			if (FD_ISSET(listener->ports[i], &readfds)) {
				if ((new_socket = accept(listener->ports[i], 
								(struct sockaddr *)&address,
								(socklen_t *)&addrlen)) < 0) {
					perror("accept\n");
					exit(EXIT_FAILURE);
				}

				/* Inform user of socket number - useful in send 
				* and receive actions */
				printf("New connection: \nsocket fd: \t%d\nip: \t%s\nport:\t%d\n",
						new_socket, inet_ntoa(address.sin_addr), 
						ntohs(address.sin_port));

				/* add to users */
				add_connection(listener->users, new_socket);

				if (i == 0) {
					/* internal user */
					/* generate ip */
					ip_add = allocate_address(listener->ip_allocator);

					/* check if ip available */
					while (!login_connection(listener->users, new_socket, ip_add)) {
						free(ip_add);
						ip_add = allocate_address(listener->ip_allocator);
					}

					/* generate mac */
					mac_add = gen_mac(listener->mac_allocator);
					packet = new_packet(LOGIN, ip_add, NULL, ip_add, 8001, 8001);
					packet->header.dst_mac[0] = mac_add[0];
					packet->header.dst_mac[1] = mac_add[1];
					packet->header.dst_mac[2] = mac_add[2];
					packet->header.dst_mac[3] = mac_add[3];
					packet->header.dst_mac[4] = mac_add[4];
					packet->header.dst_mac[5] = mac_add[5];

					/* send to user */
					send_packet(packet, new_socket);
					free_packet(packet);
					packet = NULL;
					free(ip_add);
					ip_add = NULL;
					
				} else {
					/* external user */
					/* generate mac */
					mac_add = gen_mac(listener->mac_allocator);
					packet = new_packet(LOGIN, NULL, NULL, NULL, 8001, 8001);
					packet->header.dst_mac[0] = mac_add[0];
					packet->header.dst_mac[1] = mac_add[1];
					packet->header.dst_mac[2] = mac_add[2];
					packet->header.dst_mac[3] = mac_add[3];
					packet->header.dst_mac[4] = mac_add[4];
					packet->header.dst_mac[5] = mac_add[5];

					/* send to user */
					send_packet(packet, new_socket);
					free_packet(packet);
					packet = NULL;
					free(mac_add);
					mac_add = NULL;
				}
			}
		}

		/* IO on other sockets */
		for (n = online_list2->head; n; n = n->next) {
			sd = (int)((long)n->data);
			if (sd <= 0) {
				printf("this shouldn't be possible when translating a name to an sd.\n");
				continue;
			}
			if (FD_ISSET(sd, &readfds)) {
				/* boom */
				packet = NULL;
				packet = receive_packet(sd);
				if (!packet) {
					remove_channel(listener->users, sd);
					push_user_list(listener->speaker);
					/*
					close(sd);
					*/
				} else if (packet->code == QUIT) {
					remove_channel(listener->users, sd);
					push_user_list(listener->speaker);
					close(sd);
				} else if (packet->code == SEND) {
					printf("adding packet to queue\n");
					add_packet_to_queue(listener->speaker, packet);
					packet = NULL;
				} else if (packet->code == ECHO) {
					send_packet(packet, sd);
				} else if (packet->code == BROADCAST) {
					broadcast(listener->speaker, packet);
				} else if (packet->code == LOGIN) {
					printf("got login packet\n");

					if (is_private_address(packet->header.src_ip)) {
						printf("Invalid external ip address\n");
						p = new_packet(SEND, null_address, listen_strdup("denial"), packet->header.src_ip, 8002, packet->header.src_port);
						send_packet(p, sd);
						free_packet(p);
						p = NULL;
					} else if ((check_user_password(packet->header.src_ip, packet->data)) && 
							login_connection(listener->users, sd, packet->header.src_ip)) {
						push_user_list(listener->speaker);
						/* !!!!!!!!!!!!!! */
						p = new_packet(SEND, null_address, listen_strdup("accept"), packet->header.src_ip, 8002, packet->header.src_port);
						send_packet(p, sd);
						free_packet(p);
						p = NULL;
					} else {
						/* !!!!!!!!!!!!!! */
						p = new_packet(SEND, null_address, listen_strdup("denial"), packet->header.src_ip, 8002, packet->header.src_port);
						send_packet(p, sd);
						free_packet(p);
						p = NULL;
						close(sd);
						printf("closed\n");
						fd_hashset_remove(listener->users->sockets, sd);
						printf("removed\n");
					}

				} else if (packet->code == GET_ULIST) {
					add_packet_to_queue(listener->speaker, packet);
					packet = NULL;
				} else {
					fprintf(stderr, "Packet with code %d came.  this is weird\n", 
							packet->code);
				}
				if (packet) {
					free_packet(packet);
				}
			}
		}
		free_queue(online_list2);
		online_list2 = NULL;
	}
}

/* at this point not implemented */
int check_user_password(unsigned char *name, char *pw)
{
	if ((char *)name == pw) {
		return TRUE;
	} else {
		return TRUE;
	}
}

/* Ansi doesn't define strdup */
char *listen_strdup(char *s)
{
	char *c = malloc(strlen(s) + 1);
	int i = 0;
	int j = 0;

	while((c[i++] = s[j++]));

	return c;
}


unsigned char *listen_ipdup(unsigned char *s)
{
	int i;
	int len = 4;
	unsigned char *c = malloc(len * sizeof(unsigned char));

	for (i = 0; i < len; i++) {
		c[i] = s[i];
	}
	return c;
}
