#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "serializer.h"
#include "packet.h"

#ifdef DEBUG
#endif

/*** Helper Function Prototypes ******************************************/

int read_int_from_buffer(char *buffer, int *global_index);
char *read_string_from_buffer(char *buffer, int *global_index, int length);
unsigned char *read_ip_from_buffer(char *buffer, int *global_index);
int cmp(void *a, void *b);

void write_int32_to_buffer(char *buffer, int *global_index, int32_t integer);
void write_int16_to_buffer(char *buffer, int *global_index, int16_t integer);
void write_string_to_buffer(char *buffer, int *global_index, int length, char *string);
void write_n_bytes_to_buffer(char *buffer, int *global_index, int n, unsigned char *bytes);

/*** Functions ***********************************************************/

/** Take a packet and return a byte buffer representation of it.
 *
 * @param[in]  packet:	The packet to serialze.
 * @param[out] psize:	The size of the serialized packet in bytes.
 *
 * @return The byte buffer of the serialized packet.
 */
char *serialize(packet_t *packet, int *psize) 
{
	int global_index = 0;
	/*
	int len;
	*/
	int size = 0;
	int header_size = 0;
	char *buffer = NULL;
	node_t *n = NULL;

	/* headers: ethernet + tcp */
	header_size = 22 + 20 + 20;
	

	/* payload sizes */
	/* code */
	size += sizeof(int);
	/* name */
	size += sizeof(int);
	size += packet->name_len * 2;
	/* data */
	size += sizeof(int);
	size += packet->data_len * 2;
	/* to */
	size += sizeof(int);
	size += packet->to_len * 2;
	/* list */
	size += sizeof(int);
	size += packet->list_size;

	*psize = size + sizeof(int) + header_size;

	/*
	buffer = malloc(size + sizeof(int));
	*/
	buffer = malloc(header_size + sizeof(int) + size);
	if (!buffer) {
		return NULL;
	}


	/* TODO: work out checksum, etc */
	/* header */
	write_n_bytes_to_buffer(buffer, &global_index, 8, packet->header.eth_preamble);
	write_n_bytes_to_buffer(buffer, &global_index, 6, packet->header.dst_mac);
	write_n_bytes_to_buffer(buffer, &global_index, 6, packet->header.src_mac);
	write_n_bytes_to_buffer(buffer, &global_index, 2, packet->header.ethernet_type);
	
	write_n_bytes_to_buffer(buffer, &global_index, 1, &(packet->header.version_ihl));
	write_n_bytes_to_buffer(buffer, &global_index, 1, &(packet->header.dscp_ecn));
	write_n_bytes_to_buffer(buffer, &global_index, 2, packet->header.total_length);

	write_n_bytes_to_buffer(buffer, &global_index, 2, packet->header.identification);
	write_n_bytes_to_buffer(buffer, &global_index, 2, packet->header.flags_fragmentoffset);

	write_n_bytes_to_buffer(buffer, &global_index, 1, &(packet->header.time_to_live));
	write_n_bytes_to_buffer(buffer, &global_index, 1, &(packet->header.protocol));
	write_n_bytes_to_buffer(buffer, &global_index, 2, packet->header.headerchecksum);

	write_n_bytes_to_buffer(buffer, &global_index, 4, packet->header.dst_ip);
	write_n_bytes_to_buffer(buffer, &global_index, 4, packet->header.src_ip);

	write_int16_to_buffer(buffer, &global_index, packet->header.dst_port);
	write_int16_to_buffer(buffer, &global_index, packet->header.src_port);

	write_int32_to_buffer(buffer, &global_index, packet->header.sequence_no);
	write_int32_to_buffer(buffer, &global_index, packet->header.ack_no);

	write_n_bytes_to_buffer(buffer, &global_index, 2, packet->header.data_offset_reserved_flags);
	write_int16_to_buffer(buffer, &global_index, packet->header.window_size);


	write_int16_to_buffer(buffer, &global_index, packet->header.tcpchecksum);
	write_int16_to_buffer(buffer, &global_index, packet->header.urgent_pointer);
	
	write_int32_to_buffer(buffer, &global_index, size);

	write_int32_to_buffer(buffer, &global_index, packet->code);

	if (packet->name) {
		write_int32_to_buffer(buffer, &global_index, packet->name_len);
		write_string_to_buffer(buffer, &global_index, packet->name_len, packet->name);
	} else {
		write_int32_to_buffer(buffer, &global_index, 0);
	}

	if (packet->data) {
		write_int32_to_buffer(buffer, &global_index, packet->data_len);
		write_string_to_buffer(buffer, &global_index, packet->data_len, packet->data);
	} else {
		write_int32_to_buffer(buffer, &global_index, 0);
	}

	if (packet->to) {
		write_int32_to_buffer(buffer, &global_index, packet->to_len);
		write_string_to_buffer(buffer, &global_index, packet->to_len, packet->to);
	} else {
		write_int32_to_buffer(buffer, &global_index, 0);
	}

	if (packet->users) {
		write_int32_to_buffer(buffer, &global_index, packet->list_len);
		for (n = packet->users->head; n; n = n->next) {
			/*
			len = (int)strlen((char *)n->data);
			write_int32_to_buffer(buffer, &global_index, len);
			write_string_to_buffer(buffer, &global_index, len, (char *)n->data);
			*/
			write_n_bytes_to_buffer(buffer, &global_index, 4, n->data);
		}
	} else {
		write_int32_to_buffer(buffer, &global_index, 0);
	}

	return buffer;
}

/**
 * Take a byte buffer and deserialize it to a packet struct.
 *
 * @param[in] bytes: The byte buffer describing the packet.
 *
 * @return The packet structure after deserializing.
 */
packet_t *deserialize(char *bytes, p_header_t *header)
{
	packet_t *packet;
	int global_index = 0;
	int i			 = 0;
	/*
	int slen		 = 0;
	*/
	unsigned char *string	 = NULL;

	int name_len	= 0;
	int data_len	= 0;
	int to_len		= 0;
	int list_len	= 0;
	int list_size	= 0;
	
	int code   = -1;
	char *name = NULL;
	char *data = NULL;
	char *to   = NULL;
	queue_t *users = NULL;

	code = read_int_from_buffer(bytes, &global_index);

	name_len = read_int_from_buffer(bytes, &global_index);
	if (name_len) {
		name = read_string_from_buffer(bytes, &global_index, name_len);
	}

	data_len = read_int_from_buffer(bytes, &global_index);
	if (data_len) {
		data = read_string_from_buffer(bytes, &global_index, data_len);
	}

	to_len = read_int_from_buffer(bytes, &global_index);
	if (to_len) {
		to = read_string_from_buffer(bytes, &global_index, to_len);
	}

	list_len = read_int_from_buffer(bytes, &global_index);
	if (list_len) {
		init_queue(&users, cmp, free);
	}
	for (i = 0; i < list_len; i++) {
		/*
		slen = read_int_from_buffer(bytes, &global_index);
		*/
		list_size += 4;
		string = read_ip_from_buffer(bytes, &global_index);
		insert_node(users, string);
	}

	packet = new_empty_packet();
	packet->header = *header;
	packet->code = code;

	packet->name_len = name_len;
	packet->name = name;

	packet->data_len = data_len;
	packet->data = data;

	packet->to_len = to_len;
	packet->to = to;

	packet->list_len = list_len;
	packet->list_size = list_size;
	packet->users = users;

	return packet;
}

/*** Helper Functions ****************************************************/

int read_int_from_buffer(char *bytes, int *global_index) 
{
	int *iptr = (int *)(bytes + *global_index);
	int read_val;

	read_val = ntohl(*iptr);
	*global_index += (int)(sizeof(int));

	return read_val;
}

char *read_string_from_buffer(char *bytes, int *global_index, int length)
{
	int i = 0;
	char *string = NULL;
	short *temp = 0;;

	string = malloc(length + 1);

	for (i = 0; i < length; i++) {
		temp = (short *)&bytes[*global_index + (2 * i)];
		string[i] = (char) ntohs(*temp);
	}

	string[length] = '\0';
	*global_index += length * 2;
	return string;
}

unsigned char *read_ip_from_buffer(char *buffer, int *global_index)
{
	int i = 0;
	unsigned char *ip = NULL;

	ip = malloc(4);

	for (i = 0; i < 4; i++) {
		ip[i] = (unsigned char)buffer[*global_index + i];
	}

	*global_index += 4;
	return ip;
}

int cmp(void *a, void *b) 
{
	if (a == b) {
		return 0;
	} else {
		return 0;
	}
}

void write_int32_to_buffer(char *buffer, int *global_index, int32_t integer)
{
	int32_t *iptr = NULL;
	iptr = (int32_t *)(buffer + *global_index);
	*iptr = htonl(integer);
	*global_index += sizeof(int32_t);
}

void write_int16_to_buffer(char *buffer, int *global_index, int16_t integer)
{
	int16_t *iptr = NULL;
	iptr = (int16_t *)(buffer + *global_index);
	*iptr = htons(integer);
	*global_index += sizeof(int16_t);
}


void write_string_to_buffer(char *buffer, int *global_index, int length, char *string)
{
	int i;
	short *ch;
	for (i = 0; i < length; i++) {
		ch = (short *)(buffer + *global_index + (2 * i));
		*ch = htons((short)string[i]);
		/*
		buffer[*global_index + (2 * i)] = ch;
		*/
	}
	*global_index += i * 2;
}

void write_n_bytes_to_buffer(char *buffer, int *global_index, int n, unsigned char *bytes)
{
	int i;
	unsigned char *ch;
	for (i = 0; i < n; i++) {
		ch = (unsigned char *)(buffer + *global_index + i);
		*ch = bytes[i];
	}
	*global_index += i;

}
