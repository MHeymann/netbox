#ifndef PACKET_H
#define PACKET_H
#include "../queue/queue.h"

/*** struct description **************************************************/

typedef struct packet {
	
	unsigned char eth_preable[8];
	unsigned char dst_mac[6];
	unsigned char src_mac[6];
	unsigned char ethernet_type[2];

	unsigned char version_ihl;
	unsigned char dscp_ecn;
	unsigned char total_length[2];

	unsigned char identification[2];
	unsigned char flags_fragmentoffset[2];

	unsigned char time_to_live;
	unsigned char protocol;
	unsigned char headerchecksum[2];

	unsigned char dst_ip[4];
	unsigned char src_ip[4];

	int code;			/* The code specifying the packet function. */
	int name_len;		/* The number of characters in the name field */
	char *name;			/* The username of the sending client */
	int data_len;		/* The number of characters in the data field */ 
	char *data;			/* The data to be sent in the packet */
	int to_len;			/* The number of characters in the to field */
	char *to;			/* The username of the receiving client */
	int list_len;		/* The number of names in users */
	int list_size;		/* The number of bytes taken up by the list */
	queue_t *users;		/* The names to be carried in this packet */

	unsigned char frame_check_sequence[4];
	/* End of Frame */

} packet_t;

/*** Function Prototypes *************************************************/

/**
 * Malloc heap space for a new generic packet.  
 * All values initialized to NULL, 0 or -1.
 *
 * @return The newly allocated space.
 */
packet_t *new_empty_packet();

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
packet_t *new_packet(int code, char *name, char *data, char*to);

/** 
 * Free the a given packet.
 * 
 * @param[in] p: A pointer to the packet to free.
 */
void free_packet(void *p);

/**
 * Set the userlist to the packet, updating values as needed. 
 *
 * @param[in] packet:	The packet to set the list to.
 * @param[in] users:	The queue structure with the names users that
 *						the packet must carry.
 */
void set_user_list(packet_t *packet, queue_t *users);

/**
 * Get the code that describes the function of a given packet.
 *
 * @param[in] packet: The packet for which the code is required.
 * 
 * @return	The integer value describing the function of the packet,
 *			as specified in code.h
 */
int get_code(packet_t *packet);

/**
 * Set the code of a given packet.
 *
 * @param[in] packet:	The packet for which the code must
 *						be set.
 * @param[in] code:		The code to set in the packet.
 */
void set_code(packet_t *packet, int code);

/**
 * Get the data field of the given packet.
 *
 * @param[in] packet:	The packet from which the data must be
 *						extracted.
 *
 * @return A pointer to the data in the packet. 
 */
char *get_data(packet_t *packet);

/**
 * Set the data field of the given packet.
 *
 * @param[in] packet:	The packet from which the data must be set.
 * @param[in] data:		A pointer to the data to be set.
 */
void set_data(packet_t *packet, char *data);

/**
 * Send a given packet over the socket specified by fd.
 *
 * @param[in] packet:	A pointer to the packet to be sent.
 * @param[in] fd:		A file descriptor of the socket over
 *						which the packet must be sent.
 */
void send_packet(packet_t *packet, int fd);

/**
 * Receive data from a given fd and deserialize the data to a packet.
 *
 * @param[in] fd:	A file descriptor of the socket to receive the 
 *					data over.
 * @return The new packet that was received.
 */
packet_t *receive_packet(int fd);

#endif
