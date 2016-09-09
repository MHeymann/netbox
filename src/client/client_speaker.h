#ifndef CLIENT_SPEAKER_H
#define CLIENT_SPEAKER_H


#define TRUE	1
#define FALSE	0

/*** Struct Definitions **************************************************/

typedef struct client_speaker {
	char *name;		/* The username of the client */
	char *hostname; /* The ip address of the server */
	int port;		/* The port of the server */
	int sd;			/* The file descriptor of the socket */
} client_speaker_t;

/*** Function Prototypes *************************************************/

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
client_speaker_t *new_client_speaker(char *name, char *hostname, int port);

/**
 * Free a struct client_speaker.
 *
 * @param[in] speaker: The datastructure to be free'd.
 */
void free_client_speaker(client_speaker_t *speaker);

/**
 * Request a list of online users from the server.
 *
 * @param[in] speaker:	The datastructure with the socket descriptor the
 *						request must be sent to.
 */
void get_online_names(client_speaker_t *speaker);

/**
 * Send a string as a message to another user.
 * @param[in] speaker:	The struct containing the socket descriptor to
 *						be sent to.
 * @param[in] s:		The string to be sent as a message.
 * @param[in] to:		The username of the recipient.
 */
int send_string(client_speaker_t *speaker, char *s, char *to);

/**
 * Send a string to the server to be echoed back. 
 * @param[in] speaker:	The struct containing the socket descriptor to
 *						be sent to.
 * @param[in] s:		The message to be echoed.
 * @return TRUE(1) if successful, FALSE(0) otherwise.
 */
int echo_string(client_speaker_t *speaker, char *s);

/**
 * Broadcast a string to all other users.
 *
 * @param[in] speaker:	The struct containing the socket descriptor to
 *						be sent to.
 * @param[in] s:		The message to be broadcast.
 *
 * @return TRUE(1) if successful, FALSE(0) otherwise.
 */
int broadcast_string(client_speaker_t *speaker, char *s);

/**
 * return the filed descriptor of the speaker's socket.
 *
 * @param[in] speaker: the speaker whose socket is required.
 *
 * @return An integer value, a file descriptor of the socket.
 */
int get_speaker_sd(client_speaker_t *speaker);

/**
 * Create a connection to the server and log in.
 *
 * @param[in] speaker:	The speaker that must connect.
 * @param[in] pw:		The password to give to the server.
 *
 * @return TRUE(1) when successful and FALSE(0) when not.
 */
int speaker_login(client_speaker_t *speaker, char *pw);

/**
 * End the current session by notifying the server and killing 
 * the connection.
 *
 * @param[in] speaker: The speaker of the client to log off. 
 *
 * @return	TRUE(1) when successful and FALSE(0) when 
 *			something goes wrong.
 */
int speaker_logoff(client_speaker_t *speaker);

#endif
