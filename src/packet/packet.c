#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "packet.h"
#include "serializer.h"
#include "../queue/queue.h"

/*** Helper Function Prototypes ******************************************/

int cmp_strings(void *a, void *b);
char *strdup(char *s);

/*** Functions ***********************************************************/

/**
 * Malloc heap space for a new generic packet.  
 * All values initialized to NULL, 0 or -1.
 *
 * @return The newly allocated space.
 */
packet_t *new_empty_packet() 
{
	int i;
	packet_t *packet = NULL;
	packet = malloc(sizeof(packet_t));

	if (!packet) {
		return NULL;
	}

	/* preamble */
	for (i = 0; i < 7; i++) {
		packet->eth_preable[i] = (unsigned char)170;
	}
	/* SFD - Start frame delimiter */
	packet->eth_preable[7] = (unsigned char)171;

	bzero(packet->dst_mac, 6);
	bzero(packet->src_mac, 6);
	packet->ethernet_type[0] = (unsigned char)16;
	packet->ethernet_type[1] = '\0';

	packet->version_ihl = '\0';
	packet->version_ihl += (unsigned char)64;
	packet->version_ihl += (unsigned char)5;
	packet->dscp_ecn = (unsigned char)2;
	packet->total_length[0] = (unsigned char)0;
	packet->total_length[1] = (unsigned char)20;

	packet->identification[0] = '\0';
	packet->identification[1] = '\0';
	packet->flags_fragmentoffset[0] = '\0';
	packet->flags_fragmentoffset[1] = '\0';

	packet->time_to_live = (unsigned char)255;
	packet->protocol = (unsigned char)6;
	packet->headerchecksum[0] = '\0';
	packet->headerchecksum[1] = '\0';

	bzero(packet->src_ip, 4);
	bzero(packet->dst_ip, 4);

	packet->code = -1;

	packet->name = NULL; 
	packet->name_len = 0;

	packet->data = NULL;
	packet->data_len = 0;

	packet->to = NULL;
	packet->to_len =  0;

	packet->users = NULL;
	packet->list_len = 0;
	packet->list_size = 0;

	return packet;
}

/** 
 * Malloc heap space for a new packet.
 *
 * @param[in] code: An integer describing the function of the packet.
 *					See code.h.
 * @param[in] name: The username of the sender.
 * @param[in] data: The message to send.
 * @param[in] to:	The message to send.
 *
 * @return	The memory address of the newly allocated packet, or NULL
 *			if something goes wrong.
 */
packet_t *new_packet(int code, char *name, char *data, char*to)
{
	packet_t *packet = NULL;
	packet = new_empty_packet();
	/*
	packet = malloc(sizeof(packet_t));
	*/
	if (!packet) {
		return NULL;
	}

	packet->code = code;

	if (name) {
		packet->name = name; 
		packet->name_len = strlen(name);
	} else {
		packet->name = NULL;
		packet->name_len = 0;
	}

	if (data) {
		packet->data = data;
		packet->data_len = strlen(data);
	} else {
		packet->data = NULL;
		packet->data_len = 0;
	}

	if (to) {
		packet->to = to;
		packet->to_len =  strlen(to);
	} else {
		packet->to = NULL;
		packet->to_len =  0;
	}

	packet->users = NULL;
	packet->list_len = 0;
	packet->list_size = 0;

	return packet;
}

/** 
 * Free the a given packet.
 * 
 * @param[in] p: A pointer to the packet to free.
 */
void free_packet(void *p)
{
	packet_t *packet = (packet_t *)p;
	if (!packet) {
		fprintf(stderr, "Null pointer was given to free");
		return;
	}
	if (packet->name) {
		free(packet->name);
		packet->name = NULL;
		packet->name_len = -1;
	}
	if (packet->data) {
		free(packet->data);
		packet->data = NULL;
		packet->data_len = -1;
	}
	if (packet->to) {
		free(packet->to);
		packet->to = NULL;
		packet->to_len = -1;
	}
	if (packet->users) {
		free_queue(packet->users);
		packet->users = NULL;
		packet->list_len = -1;
		packet->list_size = -1;
	}
	free(p);
}

/**
 * Set the userlist to the packet, updating values as needed. 
 *
 * @param[in] packet:	The packet to set the list to.
 * @param[in] users:	The queue structure with the names users that
 *						the packet must carry.
 */
void set_user_list(packet_t *p, queue_t *users) 
{
	node_t *n = NULL;
	queue_t *cusers = NULL;
	if (p->users) {
		free_queue(p->users);
		p->users = NULL;
	}

	init_queue(&cusers, cmp_strings, free);

	p->list_len = get_node_count(users);
	p->list_size = 0;
	for (n = users->head; n; n = n->next) {
		p->list_size += sizeof(int);
		if (n->data) {
			p->list_size += strlen((char *)n->data) * 2;
			insert_node(cusers, strdup((char *)n->data));
		} else {
			fprintf(stderr, "fault in queue nodes!!\n");
		}
	}
	p->users = cusers;
}

/**
 * Get the code that describes the function of a given packet.
 *
 * @param[in] packet: The packet for which the code is required.
 * 
 * @return	The integer value describing the function of the packet,
 *			as specified in code.h
 */
int get_code(packet_t *p) 
{
	return p->code;
}

/**
 * Set the code of a given packet.
 *
 * @param[in] packet:	The packet for which the code must
 *						be set.
 * @param[in] code:		The code to set in the packet.
 */
void set_code(packet_t *p, int code)
{
	p->code = code;
}

/**
 * Get the data field of the given packet.
 *
 * @param[in] packet:	The packet from which the data must be
 *						extracted.
 *
 * @return A pointer to the data in the packet. 
 */
char *get_data(packet_t *p)
{
	return p->data;
}

/**
 * Set the data field of the given packet.
 *
 * @param[in] packet:	The packet from which the data must be set.
 * @param[in] data:		A pointer to the data to be set.
 */
void set_data(packet_t *p, char *data)
{
	if (p->data) {
		free(p->data);
		p->data = NULL;
	}
	p->data = data;
}

/**
 * Send a given packet over the socket specified by fd.
 *
 * @param[in] packet:	A pointer to the packet to be sent.
 * @param[in] fd:		A file descriptor of the socket over
 *						which the packet must be sent.
 */
void send_packet(packet_t *packet, int fd)
{
	int size;
	int write_bytes;
	int i;
	char *buffer = NULL;
	char sbuffer[4];
	int *iptr = (int *)sbuffer;

	buffer = serialize(packet, &size);
	
	*iptr = htonl(size);

	write_bytes = write(fd, sbuffer, 4);
	for (i = 0; i < size;) {
		write_bytes = write(fd, (buffer + i), size - i);
		i += write_bytes;
	}
	free(buffer);
}

/**
 * Receive data from a given fd and deserialize the data to a packet.
 *
 * @param[in] fd:	A file descriptor of the socket to receive the 
 *					data over.
 * @return The new packet that was received.
 */
packet_t *receive_packet(int fd) 
{
	/* to be continued... */
	packet_t *packet = NULL;
	int size = 0;
	int r = 0;
	int i = 0;
	char sizebuffer[sizeof(int)];
	char *b = (char *)sizebuffer;
	int *intp;

	r = read(fd, (void *)b, sizeof(int));

	if (r == 0) {
#ifdef PDEBUG
		printf("0 disconnect*************\n");
		printf("Does this ever happen??*************\n");
#endif
		close(fd);
		return NULL;
	} else if (r == -1) {
#ifdef PDEBUG
		printf("-1 disconnect*************\n");
#endif
		close(fd);
		return NULL;
	}


	intp = (int *)b;
	size = ntohl(*intp);
	
	if (size <= 0) {
#ifdef PDEBUG
		printf("this is objectively weird. Inside receive_packet\n");
#endif
		return NULL;
	}

	b = NULL;
	b = malloc(size);
	if (!b) {
		fprintf(stderr, "Failed to malloc a buffer in receive_packet\n");
		return NULL;
	}
	for (i = 0; i < size; i++) {
		r = read(fd, (b + i), size - i);
		printf("Read %d bytes of %d\n", r, size);
		i += r;
		if (r == -1) {
			/*
			fprintf(stderr, "Potentially weird!\n");
			*/
			/*
			close(fd);
			*/
			printf("read -1\n");
			break;
		} else if (r == 0) {
			printf("read 0\n");
		}
	}

	packet = deserialize(b);
	free(b);

	return packet;
	
}

/*** Helper Functions ****************************************************/

int cmp_strings(void *a, void *b)
{
	return strcmp((char *)a, (char *)b);
}

char *strdup(char *s)
{
	char *c = malloc(strlen(s) + 1);
	int i = 0;
	int j = 0;

	while((c[i++] = s[j++]));

	return c;
}
