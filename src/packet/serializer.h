#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "packet.h"

/** Take a packet and return a byte buffer representation of it.
 *
 * @param[in]  packet:	The packet to serialze.
 * @param[out] psize:	The size of the serialized packet in bytes.
 *
 * @return The byte buffer of the serialized packet.
 */
char *serialize(packet_t *packet, int *psize);

/**
 * Take a byte buffer and deserialize it to a packet struct.
 *
 * @param[in] bytes: The byte buffer describing the packet.
 *
 * @return The packet structure after deserializing.
 */
packet_t *deserialize(char *bytes, p_header_t *header); 

#endif
