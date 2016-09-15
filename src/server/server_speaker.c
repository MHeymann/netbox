#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../packet/code.h"
#include "server_speaker.h"
#include "../address/address_alloc.h"
/*
typedef struct speaker {
	users_t *users;
	sem_t *queue_sem;
	pthread_mutex_t *queue_lock;
	queue_t *q;
	unsigned char serv_ip[4]
} server_speaker_t;
*/

/*** Helper Function Prototypes ******************************************/

int cmp_dummy(void *a, void *b);
char *speak_strdup(char *s);
void speaker_go(server_speaker_t *speaker);
unsigned char *speak_ipdup(unsigned char *s);
int is_server_address(unsigned char *ip, unsigned char *sip);

/*** Functions ***********************************************************/

/**
 * Allocate heap space for the struct.
 *
 * @param[in] users: The users currently online.
 *
 * @return The new data structure.
 */
server_speaker_t *new_server_speaker(users_t *users, unsigned char *serv_ip)
{
	server_speaker_t *speaker = NULL;
	queue_t *q = NULL;

	speaker = malloc(sizeof(server_speaker_t));
	if (!speaker) {
		fprintf(stderr, "failed to malloc server_speaker\n");
		return NULL;
	}
	speaker->serv_ip[0] = serv_ip[0];
	speaker->serv_ip[1] = serv_ip[1];
	speaker->serv_ip[2] = serv_ip[2];
	speaker->serv_ip[3] = serv_ip[3];

	speaker->users = users;
	speaker->queue_sem = malloc(sizeof(sem_t));
	if (!speaker->queue_sem) {
		fprintf(stderr, "failed to malloc queue sem for server_speaker\n");
		free(speaker);
		return NULL;
	}
	sem_init(speaker->queue_sem, 0, 0);

	speaker->queue_lock = malloc(sizeof(pthread_mutex_t));
	if (!speaker->queue_lock) {
		fprintf(stderr, "failed to malloc queue lock for server_speaker\n");
		sem_destroy(speaker->queue_sem);
		free(speaker->queue_sem);
		free(speaker);
		return NULL;
	}
	pthread_mutex_init(speaker->queue_lock, NULL);

	init_queue(&q, cmp_dummy, free_packet);
	speaker->q = q;

	speaker->run_status = TRUE;
	speaker->status_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(speaker->status_lock, NULL);

	speaker->iptable = new_ipbinds();
	speaker->ip_timeout = 600;

	return speaker;
}

/**
 * Free the struct.
 *
 * @param[in] speaker: the struct to be free'd.
 */
void server_speaker_free(server_speaker_t *speaker)
{

	speaker->users = NULL;
	if (speaker->queue_sem) {
		sem_destroy(speaker->queue_sem);
		free(speaker->queue_sem);
		speaker->queue_sem = NULL;
	}
	if (speaker->queue_lock) {
		pthread_mutex_destroy(speaker->queue_lock);
		free(speaker->queue_lock);
		speaker->queue_lock = NULL;
	}
	if (speaker->q) {
		free_queue(speaker->q);
		speaker->q = NULL;
	}
	if (speaker->status_lock) {
		pthread_mutex_destroy(speaker->status_lock);
		free(speaker->status_lock);
		speaker->status_lock = NULL;
	}
	if (speaker->iptable) {
		free_ipbinds(speaker->iptable);
		speaker->iptable = NULL;
	}
	free(speaker);
}

/**
 * Add a packet to the internal queue to be processed by the speaker.
 *
 * @param[in] speaker:	The speaker thread's data structure that contains
 *						the queue.
 * @param[in] packet:	The packet datastructure that must be processed.
 */
void add_packet_to_queue(server_speaker_t *speaker, packet_t *packet)
{
	pthread_mutex_lock(speaker->queue_lock);
	insert_node(speaker->q, (void *)packet);
	pthread_mutex_unlock(speaker->queue_lock);

	sem_post(speaker->queue_sem);
}

/** 
 * Send a list of online users to all online users.
 *
 * @param[in] speaker:	The speaker used by this thread.
 */
void push_user_list(server_speaker_t *speaker)
{
	packet_t *packet = NULL;
	queue_t *ips = NULL;
	node_t *n = NULL;

	ips = get_ips(speaker->users);

	for (n = ips->head; n; n = n->next) {
		packet = NULL;
		packet = new_packet(GET_ULIST,(unsigned char *)n->data, NULL, (unsigned char *)n->data, 8001, 8001);
		set_user_list(packet, ips);

		add_packet_to_queue(speaker, packet);
	}

	free_queue(ips);
}

/**
 * Send a packet to all online users.
 *
 * @param[in] speaker:	The speaker sending packets out.
 * @param[in] packet:	The packet to be broadcast.
 */
void broadcast(server_speaker_t *speaker, packet_t *packet)
{
	packet_t *copy;
	queue_t *ips = get_ips(speaker->users);
	node_t *n = NULL;
	unsigned char *ptr;

	printf("%d.%d.%d.%d is broadcasting %s\n", 
			(int)packet->header.src_ip[0], 
			(int)packet->header.src_ip[1], 
			(int)packet->header.src_ip[2], 
			(int)packet->header.src_ip[3], 
			packet->data);
	for (n = ips->head; n; n = n->next) {
		ptr = n->data;
		printf("%d.%d.%d.%d to be added for broadcasting\n", ptr[0], ptr[1], ptr[2], ptr[3]);
		copy = NULL;
		copy = new_packet(packet->code, packet->header.src_ip, 
				speak_strdup(packet->data), (unsigned char *)n->data, 
				packet->header.src_port, packet->header.dst_port);
		add_packet_to_queue(speaker, copy);
	}
	free_queue(ips);
}

/**
 * The process to be run when creating the thread.
 *
 * @param[in] speaker:	The struct to be used for overhead by the
 *						speaker thread.
 */
void *speaker_run(void *s) 
{
	if (!s) {
		return NULL;
	} else {
		speaker_go((server_speaker_t *)s);
	}
	return NULL;
}

/**
 * Signal to the speaker thread to stop, breaking out of it's while loop.
 * 
 * @param[in] speaker: The struct to administer overhead.
 */
void speaker_stop(server_speaker_t *speaker)
{
	pthread_mutex_lock(speaker->status_lock);
	speaker->run_status = FALSE;
	pthread_mutex_unlock(speaker->status_lock);
	sem_post(speaker->queue_sem);
}
/**
 * Check whether the speaker thread is currently marked as running.
 *
 * @param[in] speaker: The struct to administer overhead.
 */
int speaker_running(server_speaker_t *speaker)
{
	int status;
	pthread_mutex_lock(speaker->status_lock);
	status = speaker->run_status;
	pthread_mutex_unlock(speaker->status_lock);
	return status;
}

void refresh_ip_binds(server_speaker_t *speaker)
{
	int currtime = (long)time(NULL);
	queue_t *q = ipbinds_get_ips(speaker->iptable);
	unsigned char *ip = NULL;

	printf("refreshing ip port table\n");

	while ((ip = pop_first(q))) {
		if (currtime - ip_get_time(speaker->iptable, ip) > speaker->ip_timeout) {
			ipbinds_remove_ip(speaker->iptable, ip);
			printf("Removed %d.%d.%d.%d\n", 
					ip[0],
					ip[1],
					ip[2],
					ip[3]
					);
		}
		free(ip);
		ip = NULL;
	}

	free_queue(q);
	q = NULL;
	printf("refresh done\n");
}


/*** Helper Functions ****************************************************/

int cmp_dummy(void *a, void *b)
{
	if ((long)a == (long)b) {
		return 1;
	} else {
		return 1;
	}
}

/* ansi */
char *speak_strdup(char *s)
{
	int i = 0, j = 0;
	char *c = malloc(strlen(s) + 1);
	while ((c[i++] = s[j++]));
	return c;
}

/* The workhorse that does the work */
void speaker_go(server_speaker_t *speaker)
{
	packet_t *packet = NULL;
	packet_t *temp = NULL;
	int port;
	unsigned char *ip;

	queue_t *online_users = NULL;
	while(TRUE) {
		/* wait for the semaphore to be increased, indicating 
		 * new activity to be processed */
		sem_wait(speaker->queue_sem);
		if (!speaker_running(speaker)) {
			break;
		}
		packet = NULL;

		pthread_mutex_lock(speaker->queue_lock);
		packet = (packet_t *)pop_first(speaker->q);
		pthread_mutex_unlock(speaker->queue_lock);

		/* handle packet according to it's code */
		if (packet->code == SEND) {
			 if ((is_private_address(packet->header.src_ip)) && (!is_private_address(packet->header.dst_ip))) {
				temp = packet;
				packet = NULL;
				if ((port = ip_get_bound_port(speaker->iptable, temp->header.src_ip)) == FALSE) {
					for (port = 1; !bind_ip_to_port(speaker->iptable, temp->header.src_ip, port); port++);
					printf("%d.%d.%d.%d bound to %d\n",
							temp->header.src_ip[0],
							temp->header.src_ip[1],
							temp->header.src_ip[2],
							temp->header.src_ip[3],
							port
							);
				}
				printf("port %d used to send out of\n", port);

				packet = new_packet(SEND, speaker->serv_ip, speak_strdup(temp->data), temp->header.dst_ip, port, temp->header.dst_port);
				/*
				packet->header.src_port = port;
				packet->header.dst_port = temp->header.dst_port;
				*/
				free_packet(temp);
			} else if ((!is_private_address(packet->header.src_ip)) && (is_server_address(packet->header.dst_ip, speaker->serv_ip))) {
				if ((ip = port_get_bound_ip(speaker->iptable, packet->header.dst_port)) == NULL) {
					printf("This port is unbound.\n");
					free_packet(packet);
					packet = NULL;
				} else {
					temp = packet;
					packet = new_packet(SEND, temp->header.src_ip, speak_strdup(temp->data), ip, temp->header.src_port, 8001);

					packet->header.src_port = temp->header.src_port;
					packet->header.dst_port = 8001;
					free_packet(temp);
					temp = NULL;
					free(ip);
					ip = NULL;
				}
			} else if ((!is_private_address(packet->header.src_ip)) && (is_private_address(packet->header.dst_ip))) {
				printf("Invalid target address from external domain\n");
				printf("Dropping packet\n");
				free_packet(packet);
				packet = NULL;
			} else if ((!is_private_address(packet->header.src_ip)) && (!is_private_address(packet->header.dst_ip))) {
				printf("Dropping packet, not allowed to route from extern to extern\n");
				free_packet(packet);
				packet = NULL;
			} else {
				/* internal to internal, nothing to do */
			}
			if (packet) {
				printf("Sending message: %d.%d.%d.%d -> %d.%d.%d.%d %s\n", 
					(int)packet->header.src_ip[0], 
					(int)packet->header.src_ip[1], 
					(int)packet->header.src_ip[2], 
					(int)packet->header.src_ip[3], 
					(int)packet->header.dst_ip[0], 
					(int)packet->header.dst_ip[1], 
					(int)packet->header.dst_ip[2], 
					(int)packet->header.dst_ip[3], 
					packet->data);
			} else {
				printf("dropped packet\n");
			}
		} else if (packet->code == GET_ULIST) {
			online_users = NULL;
			if (packet->users == NULL) {
				online_users = get_ips(speaker->users);
				set_user_list(packet, online_users);
				free_queue(online_users);
			}
			/*
			packet->name = NULL;
			packet->name_len = 0;
			*/
			packet->header.dst_ip[0] = packet->header.src_ip[0];
			packet->header.dst_ip[1] = packet->header.src_ip[1];
			packet->header.dst_ip[2] = packet->header.src_ip[2];
			packet->header.dst_ip[3] = packet->header.src_ip[3];
			printf("Sending list of online users to %d.%d.%d.%d\n", 
					packet->header.src_ip[0], 
					packet->header.src_ip[1], 
					packet->header.src_ip[2], 
					packet->header.src_ip[3]);
		}
		if (packet) {
			users_send_packet(speaker->users, packet);
			free_packet(packet);
			packet = NULL;
		}
	}
}



unsigned char *speak_ipdup(unsigned char *s)
{
	int i;
	int len = 4;
	unsigned char *c = malloc(len * sizeof(unsigned char));

	for (i = 0; i < len; i++) {
		c[i] = s[i];
	}
	return c;
}

int is_server_address(unsigned char *ip, unsigned char *sip) 
{
	int i;
	for (i = 0; i < 4; i++) {
		if (ip[i] != sip[i]) {
			return FALSE;
		}
	}
	return TRUE;
}
