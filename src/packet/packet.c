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
char *packet_strdup(char *s);
unsigned char *packet_ipdup(unsigned char *s);

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
		packet->header.eth_preamble[i] = (unsigned char)170;
	}
	/* SFD - Start frame delimiter */
	packet->header.eth_preamble[7] = (unsigned char)171;

	bzero(packet->header.dst_mac, 6);
	bzero(packet->header.src_mac, 6);
	packet->header.ethernet_type[0] = (unsigned char)16;
	packet->header.ethernet_type[1] = '\0';

	packet->header.version_ihl = '\0';
	packet->header.version_ihl += (unsigned char)64;
	packet->header.version_ihl += (unsigned char)5;
	packet->header.dscp_ecn = (unsigned char)2;
	packet->header.total_length[0] = (unsigned char)0;
	packet->header.total_length[1] = (unsigned char)20;

	packet->header.identification[0] = '\0';
	packet->header.identification[1] = '\0';
	packet->header.flags_fragmentoffset[0] = '\0';
	packet->header.flags_fragmentoffset[1] = '\0';

	packet->header.time_to_live = (unsigned char)255;
	packet->header.protocol = (unsigned char)6;
	packet->header.headerchecksum[0] = '\0';
	packet->header.headerchecksum[1] = '\0';

	bzero(packet->header.src_ip, 4);
	bzero(packet->header.dst_ip, 4);

	packet->header.src_port = 0;
	packet->header.dst_port = 0;

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
packet_t *new_packet(int code, unsigned char *src_ip, char *data, unsigned char *dst_ip, int src_port, int dst_port)
{
	int i;
	packet_t *packet = NULL;
	packet = new_empty_packet();
	/*
	packet = malloc(sizeof(packet_t));
	*/
	if (!packet) {
		return NULL;
	}

	packet->header.src_port = src_port;
	packet->header.dst_port = dst_port;
	packet->code = code;

	if (src_ip) {
		for (i = 0; i < 4; i++) {
			packet->header.src_ip[i] = src_ip[i]; 
		}
	} else {
		for (i = 0; i < 4; i++) {
			packet->header.src_ip[i] = '\0';
		}
	}
	/*
	packet->name = NULL;
	packet->name_len = 0;
	*/

	if (data) {
		packet->data = data;
		packet->data_len = strlen(data);
	} else {
		packet->data = NULL;
		packet->data_len = 0;
	}

	if (dst_ip) {
		for (i = 0; i < 4; i++) {
			packet->header.dst_ip[i] = dst_ip[i];
		}
	} else {
		for (i = 0; i < 4; i++) {
			packet->header.dst_ip[i] = '\0';
		}
	}
	/*
	packet->to = NULL;
	packet->to_len = 0;
	*/

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
		/*
		p->list_size += sizeof(int);
		*/
		if (n->data) {
			p->list_size += 4;
			insert_node(cusers, packet_ipdup((unsigned char *)n->data));
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
	/*
	char sbuffer[4];
	*/
	/*
	int *iptr = (int *)sbuffer;
	*/

	buffer = serialize(packet, &size);
	
	/*
	*iptr = htonl(size);
	*/

	/*
	write_bytes = write(fd, sbuffer, 4);
	*/
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
	p_header_t header;
	int size = 0;
	int r = 0;
	int i = 0;
	char sizebuffer[sizeof(int)];
	char *b = (char *)sizebuffer;
	int *intp;
	int port;

	r = read(fd, (void *)(header.eth_preamble), 8);
	r = read(fd, (void *)(header.dst_mac), 6);
	r = read(fd, (void *)(header.src_mac), 6);
	r = read(fd, (void *)(header.ethernet_type), 2);

	r = read(fd, (void *)(&header.version_ihl), 1);
	r = read(fd, (void *)(&header.dscp_ecn), 1);
	r = read(fd, (void *)(header.total_length), 2);

	r = read(fd, (void *)(header.identification), 2);
	r = read(fd, (void *)(header.flags_fragmentoffset), 2);

	r = read(fd, (void *)(&header.time_to_live), 1);
	r = read(fd, (void *)(&header.protocol), 1);
	r = read(fd, (void *)(header.headerchecksum), 2);
	
	r = read(fd, (void *)(header.dst_ip), 4);
	r = read(fd, (void *)(header.src_ip), 4);

	r = read(fd, &port, sizeof(int));
	header.dst_port = ntohl(port);
	r = read(fd, &port, sizeof(int));
	header.src_port = ntohl(port);

	r = read(fd, (void *)(b), sizeof(int));
	
	if (r == 0) {
#ifdef DEBUG
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

	packet = deserialize(b, &header);
	free(b);

	return packet;
	
}

/*** Helper Functions ****************************************************/

int cmp_strings(void *a, void *b)
{
	return strcmp((char *)a, (char *)b);
}

char *packet_strdup(char *s)
{
	char *c = malloc(strlen(s) + 1);
	int i = 0;
	int j = 0;

	while((c[i++] = s[j++]));

	return c;
}


unsigned char *packet_ipdup(unsigned char *s) 
{
	int i;
	unsigned char *c = malloc(4 * sizeof(unsigned char));

	for (i = 0; i < 4; i++) {
		c[i] = s[i];
	}
	return c;
}
