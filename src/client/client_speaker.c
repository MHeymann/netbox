#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
*/
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> /* write */
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
/*
#include <select.h>
*/


#include "client_speaker.h"
#include "../packet/packet.h"
#include "../packet/code.h"

/*
typedef struct client_speaker {
	unsigned char *client_ip;
	char *hostname;
	int port;
	int sd;
} client_speaker_t;
*/

/*** Helper Function Prototypes ******************************************/

char *speaker_strdup(char *s);
unsigned char *speaker_ipdup(unsigned char *s);
int speaker_send_packet(client_speaker_t *speaker, packet_t *packet);
int connect_speaker(client_speaker_t *speaker);

/*** Functions ***********************************************************/

/**
 * Allocate heap space for the data used to guide this speaker, as defined
 * in struct client_speaker.
 * The strings are duplicated and the params provided must be free'd 
 * seperately by the caller.
 * 
 * @param[in] name:		The username of the client.
 * @param[in] hostname:	The ip address of the server.
 * @param[in] port:		The port of the server. 
 *
 * @return A pointer to a new instance of struct client_speaker.
 */
client_speaker_t *new_client_speaker(unsigned char *client_ip, char *hostname, int port)
{
	client_speaker_t *speaker = NULL;

	speaker = malloc(sizeof(client_speaker_t));
	if (speaker) {
		speaker->client_ip = speaker_ipdup(client_ip);
		speaker->hostname = speaker_strdup(hostname);
		speaker->port = port;
		speaker->sd = 0;
	} else {
		fprintf(stderr, "error allocating speaker\n");
	}

	return speaker;
}

/**
 * Free a struct client_speaker.
 *
 * @param[in] speaker: The datastructure to be free'd.
 */
void free_client_speaker(client_speaker_t *speaker)
{
	if (!speaker) {
		return;
	}
	if (speaker->client_ip) {
		free(speaker->client_ip);
		speaker->client_ip = NULL;
	}
	if (speaker->hostname) {
		free(speaker->hostname);
		speaker->hostname = NULL;
	}
	if (speaker->port) {
		speaker->port = -1;
	}
	if (speaker->sd) {
		/*
		close(speaker->sd);
		*/
		speaker->sd = -1;
	}
	free(speaker);
}

/**
 * Free a struct client_speaker.
 *
 * @param[in] speaker: The datastructure to be free'd.
 */
/*
void free_speaker(client_speaker_t *speaker)
{
	if (!speaker) {
		return;
	}
	if (speaker->name) {
		free(speaker->name);
		speaker->name = NULL;
	}
	if (speaker->hostname) {
		free(speaker->hostname);
		speaker->hostname = NULL;
	}
	speaker->port = 0;
	close(speaker->sd);
	speaker->sd = 0;

	free(speaker);
}
*/

/**
 * Request a list of online users from the server.
 *
 * @param[in] speaker:	The datastructure with the socket descriptor the
 *						request must be sent to.
 */
void get_online_names(client_speaker_t *speaker)
{
	packet_t *packet = new_packet(GET_ULIST, speaker->client_ip, NULL, NULL);
	if (packet) {
		if (!speaker_send_packet(speaker, packet)) {
			fprintf(stderr, "Failed to send packet to request user list\n");
		}
	} else {
		fprintf(stderr, "could not make a packet to request users with\n");
	}
	free_packet(packet);
}

/**
 * Send a string as a message to another user.
 * @param[in] speaker:	The struct containing the socket descriptor to
 *						be sent to.
 * @param[in] s:		The string to be sent as a message.
 * @param[in] to:		The username of the recipient.
 */
int send_string(client_speaker_t *speaker, char *s, unsigned char *dst_ip)
{
	packet_t *packet = new_packet(SEND, speaker->client_ip, 
			speaker_strdup(s), dst_ip);
	if (packet) {
		if (!speaker_send_packet(speaker, packet)) {
			fprintf(stderr, "Failed to send message packet\n");
		} else {
			free_packet(packet);
			return TRUE;
		}
	} else {
		fprintf(stderr,
				"could not make a packet to send message with\n");
	}
	free_packet(packet);
	return FALSE;
}

/**
 * Send a string to the server to be echoed back. 
 * @param[in] speaker:	The struct containing the socket descriptor to
 *						be sent to.
 * @param[in] s:		The message to be echoed.
 * @return TRUE(1) if successful, FALSE(0) otherwise.
 */
int echo_string(client_speaker_t *speaker, char *s)
{
	packet_t *packet = new_packet(ECHO, speaker->client_ip, 
			speaker_strdup(s), NULL);
	if (packet) {
		if (!speaker_send_packet(speaker, packet)) {
			fprintf(stderr, "Failed to send packet for echoing\n");
		} else {
			free_packet(packet);
			return TRUE;
		}
	} else {
		fprintf(stderr, "could not make a packet to echo message with\n");
	}
	free_packet(packet);
	return FALSE;
}

/**
 * Broadcast a string to all other users.
 *
 * @param[in] speaker:	The struct containing the socket descriptor to
 *						be sent to.
 * @param[in] s:		The message to be broadcast.
 *
 * @return TRUE(1) if successful, FALSE(0) otherwise.
 */
int broadcast_string(client_speaker_t *speaker, char *s)
{
	packet_t *packet = new_packet(BROADCAST, speaker->client_ip, 
			speaker_strdup(s), NULL);
	if (packet) {
		if (!speaker_send_packet(speaker, packet)) {
			fprintf(stderr, "Failed to send packet to broadcast with\n");
		} else {
			free_packet(packet);
			return TRUE;
		}
	} else {
		fprintf(stderr, 
				"could not make a packet to broadcast message with\n");
	}
	free_packet(packet);
	return FALSE;
}

/**
 * return the filed descriptor of the speaker's socket.
 *
 * @param[in] speaker: the speaker whose socket is required.
 *
 * @return An integer value, a file descriptor of the socket.
 */
int get_speaker_sd(client_speaker_t *speaker)
{
	return speaker->sd;
}

/**
 * Create a connection to the server and log in.
 *
 * @param[in] speaker:	The speaker that must connect.
 * @param[in] pw:		The password to give to the server.
 *
 * @return TRUE(1) when successful and FALSE(0) when not.
 */
int speaker_login(client_speaker_t *speaker, char *pw)
{
	packet_t *packet = NULL;
	fd_set readfds;
	int act;

	/*
	printf("Connecting\n");
	if (!connect_speaker(speaker)) {
		fprintf(stderr, "Failed to connect to server\n");
		return FALSE;
	}
	printf("Connected\n");
	*/
	printf("in client login function of speaker\n");

	packet = new_packet(LOGIN, speaker->client_ip, 
			speaker_strdup(pw), NULL);
	printf("%d.%d.%d.%d\n", speaker->client_ip[0], speaker->client_ip[1], speaker->client_ip[2], speaker->client_ip[3]);

	printf("Sending log in packet\n");
	if (!speaker_send_packet(speaker, packet)) {
		printf("Failed to send login details\n");
		free_packet(packet);
		return FALSE;
	}
	printf("Sent packet\n");

	free_packet(packet);
	packet = NULL;

	FD_ZERO(&readfds);
	FD_SET(speaker->sd, &readfds);
	printf("Awaiting response\n");
	act = select(speaker->sd + 1, &readfds, NULL, NULL, NULL);
	printf("Getting response\n");

	if ((act < 0) && (errno != EINTR)) {
		printf("select error\n");
	}

	if (!FD_ISSET(speaker->sd, &readfds)) {
		printf(
		"After select with only one sd set, this should be impossible\n");
	}

	packet = receive_packet(speaker->sd);

	printf("Received response\n");
	
	if (packet == NULL)	{
		return FALSE;
	}
	if (packet->data == NULL) {
		return FALSE;
	}

	if (strcmp(packet->data, "accept") == 0) {
		free_packet(packet);
		return TRUE;
	} else {
		free_packet(packet);
		return FALSE;
	}
}

/**
 * End the current session by notifying the server and killing 
 * the connection.
 *
 * @param[in] speaker: The speaker of the client to log off. 
 *
 * @return	TRUE(1) when successful and FALSE(0) when 
 *			something goes wrong.
 */
int speaker_logoff(client_speaker_t *speaker)
{
	packet_t *packet = new_packet(QUIT, speaker->client_ip, 
			NULL, NULL);
	if (packet) {
		if (!speaker_send_packet(speaker, packet)) {
			fprintf(stderr,
					"Failed to send packet to notify server of exit\n");
		} else {
			free_packet(packet);
			return TRUE;
		}
	} else {
		fprintf(stderr,
				"could not make a packet to notify server of exit\n");
	}
	free_packet(packet);

	return FALSE;
}

/*** Helper Functions ****************************************************/

/* strdup defined explicitly, as doesn't exist in ansi libraries */
char *speaker_strdup(char *s)
{
	char *c = malloc(strlen(s) + sizeof(char));
	int i = 0;
	int j = 0;

	if (!c) {
		fprintf(stderr, "memory error in speaker_strdup\n");
	} else {
		while ((c[i++] = s[j++]));
	}
	return c;
}

/* basically just wraps the same function as in packet.c */
int speaker_send_packet(client_speaker_t *speaker, packet_t *packet)
{
	send_packet(packet, speaker->sd);
	return TRUE;
}

/* Get a socket and connect it to the server */
int connect_speaker(client_speaker_t *speaker)
{
	int sockfd;
	struct sockaddr_in addr;

	/* create a new socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Problem creating socket\n");
		return FALSE;
	}
	
	/* clear the memory values of addr */
	memset(&addr, 0, sizeof(addr));
	/* set the address to the server's ip and port */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(speaker->hostname);
	addr.sin_port = htons(speaker->port);

	/* connect */
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Problem connecting to server\n");
		return FALSE;
	} else {
		speaker->sd = sockfd;
		return TRUE;
	}
}

unsigned char *speaker_ipdup(unsigned char *s) 
{
	int i;
	unsigned char *c = malloc(4 * sizeof(unsigned char));

	for (i = 0; i < 4; i++) {
		c[i] = s[i];
	}

	return c;
}
