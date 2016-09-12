#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chat_client.h"

/*** Struct Definition ***************************************************/

/* just the outline of the struct used by an instance of a chat client*/
/*
typedef struct client {
	pthread_t *listen_thread;
	client_speaker_t *speaker;
	client_listener_t *listener;
	int connected_status;
	pthread_mutex_t *connection_mutex;
	unsigned char *client_ip;
	char *hostname;
	int hostport;
} chat_client_t;
*/

/*** Helper Function Prototypes ******************************************/

char *client_strdup(char *s);
unsigned char *client_ipdup(unsigned char *s);
void broken_connection(chat_client_t *client);
int	is_internal(char *s);
void read_line(FILE *f, char *line);
void print_usage();

char ch = '\0';

/*** Main Function *******************************************************/

int main (int argc, char *argv[]) 
{
	chat_client_t *client = NULL; /* an instance of a client for methods */
	char line[MAX_LINE]; /* a buffer for when text is being entered */
	char *localhost = "localhost"; /* a string for easily comparison */
	/* strings for setting up the client */
	unsigned char client_ip[4];
	int client_ip_int[4];
	char *password = NULL;
	char *hostname = NULL;
	char *message = NULL;
	unsigned char dst_ip[4];
	int dst_ip_int[4];
	/* for strtol error checking */
	/*
	char *endptr;
	*/
	/* the port of the relevant server */
	int host_port;
	int ret;

	if (argc < 2) {
		print_usage(argv);
		exit(1);
	}

	if ((client = new_client()) == NULL) {
		fprintf(stderr, "Failed to create client\n");
		exit(1);
	}

	if (strcmp(localhost, argv[1]) == 0) {
		hostname = client_strdup("127.0.0.1");
	} else {
		hostname = client_strdup(argv[1]);
	}

	if (argc > 2) {
		if (is_internal(argv[2])) {
			host_port = 8001;
		} else {
			host_port = 8002;
		}
	} else {
		printf("Are you an internal or external client? ([I]/e) ");
		read_line(stdin, line);
		if (strlen(line) == 0) {
			host_port = 8001;
		} else if (is_internal(line)) {
			host_port = 8001;
		} else {
			host_port = 8002;
		}
	}
	goto GIVE_UNAME;

	/* get the relevant data for logging in with */
ESTABLISH_CONNECTION:
	while (client->connected_status == FALSE) {
		printf("Please provide a host address for the server: ");
		if (0 == scanf("%s", line)) {
			fprintf(stderr, "problem reading hostaddress\n");
			continue;
		}
		if (!strcmp(line, localhost)) {
			hostname = client_strdup("127.0.0.1");
		} else {
			hostname = client_strdup(line);
		}

		/*
GIVE_PORT:
*/
		printf("Are you an internal or external client? ([I]/e) ");
		read_line(stdin, line);
		if (strlen(line) == 0) {
			host_port = 8001;
		} else if (is_internal(line)) {
			host_port = 8001;
		} else {
			host_port = 8002;
		}
			
GIVE_UNAME:
		printf("Please provide a client_ip: ");
		if (4 != scanf("%d.%d.%d.%d", client_ip_int, client_ip_int + 1,
					client_ip_int + 2, client_ip_int + 3)) {
			fprintf(stderr, "problem reading client_ip\n");
			goto GIVE_UNAME;
		}
		client_ip[0] = (unsigned char)client_ip_int[0];
		client_ip[1] = (unsigned char)client_ip_int[1];
		client_ip[2] = (unsigned char)client_ip_int[2];
		client_ip[3] = (unsigned char)client_ip_int[3];
	
		printf("Please provide a password: ");
		if (0 == scanf("%s", line)) {
			fprintf(stderr, "problem reading password\n");
		}
		password = client_strdup(line);
	
		/* Flash some details for easy error spotting by user */
		printf("About to connect to %s:%d\n", hostname, host_port);
		printf("Login details\n");
		printf("\tclient_ip: %d.%d.%d.%d\n", client_ip[0], client_ip[1], 
				client_ip[2], client_ip[3]);
		printf("\tpassword: %s\n", password);

		/* Set to client.  Note that these will be free'd in free_client */
		client->client_ip = client_ipdup(client_ip);
		client->hostname = hostname;
		client->hostport = host_port;

	
		/* Create a speaker */
		if ((client->speaker = new_client_speaker(client_ip, hostname, host_port)) == NULL) {
			fprintf(stderr, "Failed to create speaker\n");
			free_chat_client(client);
			free(password);
			password = NULL;
			exit(2);
		}
		/* 
		 * Send login details over the new speaker. This entails
		 * creating a connection and also sending client_ip and password 
		 */
		if (!speaker_login(client->speaker, password)) {
			fprintf(stderr, "Failed to login\n");
			free_chat_client(client);
			client = new_client();
			free(password);
			password = NULL;
		} else {
			/* success */
			client->connected_status = TRUE;
			free(password);
			password = NULL;
		}
	} /* while connecting */

	if ((client->listener = new_client_listener(client->speaker->sd, client, client_ip)) == NULL) {
		fprintf(stderr, "Failed to create listener\n");
		free_chat_client(client);
		exit(4);
	}

	/* 
	 * this launches a pthread.  
	 * argument one is a pointer to the thread structure. 
	 * argument two contains thread attribute info, but may be NULL
	 * argument three is a pointer to the function to be run by the thread
	 * argument four is the argument to be passed to argument three's function
	 */
	pthread_create(client->listen_thread, NULL, run_client_listener, (void *)client->listener);
	
	printf("Please type 'help<enter>' for available options\n");
	/* this is just a command line interface for the user to control the program
	 * flow further on */
	while(TRUE) {
		printf(">> ");
		read_line(stdin, line);
		ret = 1;
		/*
		ret = scanf("%s", line);
		*/
		if (ret != 1) {
			printf("some problem scanning for input\n");
		} else if (strcmp(line, "help") == 0) {
			printf("The following commands are supported:\n");
			printf("\t\x1b[1;34msend\x1b[0m: \t\tSend a message.\n");
			printf("\t\x1b[1;34mbroadcast\x1b[0m: \tBroadcast a message to all users.\n");
			printf("\t\x1b[1;34mecho\x1b[0m: \t\tEcho a message to yourself.\n");
			printf("\t\x1b[1;34mlistusers\x1b[0m: \t\tList the users currently online.\n");
			printf("\t\x1b[1;34mexit\x1b[0m: \t\tShut down this chat client.\n");
			printf("\t\x1b[1;34mquit\x1b[0m: \t\tSee exit.\n");
		} else if (strcmp(line, "send") == 0) {
			printf("Type the user ip of your recipient: \n");
			printf(">> ");
			read_line(stdin, line);

			sscanf(line, "%d.%d.%d.%d", dst_ip_int, dst_ip_int + 1,
					dst_ip_int + 2, dst_ip_int + 3);
			
			dst_ip[0] = dst_ip_int[0];
			dst_ip[1] = dst_ip_int[1];
			dst_ip[2] = dst_ip_int[2];
			dst_ip[3] = dst_ip_int[3];
			/*
			to = client_strdup(line);
			if (!to) {
				printf("some error copying recipient name\n");
				continue;
			}
			*/
			printf("Type the message to be sent: \n");
			printf(">> ");
			read_line(stdin, line);
			
			message = client_strdup(line);
			if (!message) {
				printf("some error copying message\n");
				/*
				free(to);
				to = NULL;
				*/
				continue;
			}
			if (send_string(client->speaker, message, dst_ip)) {
				printf("%d.%d.%d.%d to %d.%d.%d.%d : %s\n", 
						(int)client->client_ip[0], 
						(int)client->client_ip[1], 
						(int)client->client_ip[2], 
						(int)client->client_ip[3], 
						(int)dst_ip[0], (int)dst_ip[1], 
						(int)dst_ip[2], (int)dst_ip[3],
						message);
			} else {
				printf("Some error sending message\n");
			}
			/*
			free(to);
			to = NULL;
			*/
			free(message);
			message = NULL;
		} else if (strcmp(line, "broadcast") == 0) {
			printf("Type the message to be broadcast: \n");
			printf(">> ");
			read_line(stdin, line);
			
			message = client_strdup(line);
			if (!message) {
				printf("some error copying message\n");
				continue;
			}

			if (broadcast_string(client->speaker, message)) {
			} else {
				printf("Some error broadcasting message\n");
			}

			free(message);
			message = NULL;
		} else if (strcmp(line, "echo") == 0) {
			printf("Type the message to be echoed: \n");
			printf(">> ");
			read_line(stdin, line);
			
			message = client_strdup(line);
			if (!message) {
				printf("some error copying message\n");
				continue;
			}

			if (echo_string(client->speaker, message)) {
			} else {
				printf("Some error echoing message\n");
			}

			free(message);
			message = NULL;
		} else if (strcmp(line, "listusers") == 0) {
			get_online_names(client->speaker);
		} else if (strcmp(line, "logout") == 0) {
			printf("Stopping listener\n");
			stop_listener(client->listener);
			printf("Joining listen thread\n");
			pthread_join(*(client->listen_thread), NULL);
			printf("Notifying server\n");
			speaker_logoff(client->speaker);
			free_chat_client(client);
			client = new_client();
			goto ESTABLISH_CONNECTION;
		} else if ((strcmp(line, "exit") == 0) || 
			(strcmp(line, "quit") == 0)) {
			printf("exiting client\n");
			goto STOP_THREADS;
		} else {
			printf("Please type 'help<enter>' for available options\n");
		}
	}

	/* cleanup */
	printf("speaker sending logoff\n");
	speaker_logoff(client->speaker);
STOP_THREADS:
	printf("Stopping listener\n");
	stop_listener(client->listener);
	printf("Joining listen thread\n");
	pthread_join(*(client->listen_thread), NULL);
	free_chat_client(client);

	/* normal exit */
	return 0;
}


/*** Functions ***********************************************************/

/**
 * Create a new chat client datastructure.
 *
 * Assign heapspace and initialize everything to null;
 */
chat_client_t *new_client()
{
	chat_client_t *client = malloc(sizeof(chat_client_t));
	if (!client) {
		return NULL;
	} 

	/* init all as NULL or similar */
	client->connected_status = FALSE;
	/*
	client->speak_thread = NULL;
	*/
	client->listen_thread = NULL;
	client->speaker = NULL;
	client->listener = NULL;
	client->connection_mutex = NULL;
	client->client_ip = NULL;
	client->hostname = NULL;
	client->hostport = DEFAULT_PORT;

	client->listen_thread = malloc(sizeof(pthread_t));

	if (!client->listen_thread) {
		free_chat_client(client);
		return NULL;
	}

	client->connection_mutex = malloc(sizeof(pthread_mutex_t));
	if (!client->connection_mutex) {
		free_chat_client(client);
		return NULL;
	}
	pthread_mutex_init(client->connection_mutex, NULL);

	return client;
}

/** 
 * Free the chat client structure.
 *
 * @param[in] client: A pointer to the client structure to be free'd
 */
void free_chat_client(chat_client_t *client)
{
	if (client->listen_thread) {
		/* must be joined */
		free(client->listen_thread);
		client->listen_thread = NULL;
	}
	
	/* Testing in each case whether a given value is non-zero
	 * avoids double freeing.  This depends heavily on practice of 
	 * following every free in the program by setting that pointer 
	 * to NULL, as is done in this procedure as well */
	if (client->speaker) {
		free_client_speaker(client->speaker);
		client->speaker = NULL;
	}
	if (client->listener) {
		free_client_listener(client->listener);
		client->listener = NULL;
	}
	if (client->connection_mutex) {
		pthread_mutex_destroy(client->connection_mutex);
		free(client->connection_mutex);
		client->connection_mutex = NULL;
	}
	if (client->client_ip) {
		free(client->client_ip);
		client->client_ip = NULL;
	}
	if (client->hostname) {
		free(client->hostname);
		client->hostname = NULL;
	}

	free(client);
}

/**
 * Append the text in s to the ouput interface of the client.
 * This allows for easily expanding this implementation to a gui
 * interface, and allows easy interaction between the listener and
 * such a potential gui 
 *
 * @param[in] client:	The client structure on which a potential gui
 *						might be referenced.
 * @param[in] s:		The string to be displayed
 */
void client_append(chat_client_t *client, char *s)
{
	if (!client) {
		return;
	}
	printf("%s:: %s", client->client_ip, s);
}

/** 
 *	Receive a queue of usernames and display them to the standard
 *	io interface of the client.
 *
 *	@param[in] client:	The client structure of the currently running
 *						client
 *	@param[in] users_q: The names of the users to be displayed
 */
void client_show_online_users(chat_client_t *client, queue_t *users_q)
{
	unsigned char *user_ip;
	node_t *node = NULL;
	printf("Online Users being shown to %d.%d.%d.%d:\n", 
			client->client_ip[0], 
			client->client_ip[1], 
			client->client_ip[2], 
			client->client_ip[3]);
	for (node = users_q->head; node; node = node->next) {
		user_ip = (unsigned char *)node->data;
		printf("\t- %d.%d.%d.%d\n", (int)user_ip[0], (int)user_ip[1], 
				(int)user_ip[2], (int)user_ip[3]);
	}
	printf("shown\n>> ");
}

/*** Helper Functions ****************************************************/

/**
 * Just a quick string duplication method.  
 * This is necessitated by the fact that strdup in string.h is not
 * described in ansi C and this program should be compilable with
 * that flag set.
 */
char *client_strdup(char *s)
{
	char *c = malloc(strlen(s) + 1);
	int i = 0;
	int j = 0;

	while ((c[i++] = s[j++]));

	return c;
}

unsigned char *client_ipdup(unsigned char *s) 
{
	int i;
	unsigned char *c = malloc(4);

	for (i = 0; i < 4; i++) {
		c[i] = s[i];
	}

	return c;
}

/**
 * Read a line and store it in line.
 */
void read_line(FILE *f, char *line)
{
	int i;
	for (i = 0; TRUE; i++) {
		if (i == MAX_LINE - 1) {
			break;
		}
		ch = (char)fgetc(f);
		switch (ch) {
			case EOF:
				/* fall through */
				printf("EOF");
			case '\n':
				line[i] = '\0';
				return;
			default:
				line[i] = ch;
		}
	}
	line[i] = '\0';
}

int is_internal(char *s)
{
	int retval = FALSE;
	if(s[0] == 'i') {
		retval = TRUE;
	} else if (s[0] == 'I') {
		retval = TRUE;
	} else if (s[0] == 'e') {
		retval = FALSE;
	} else if (s[0] == 'E') {
		retval = FALSE;
	}
	return retval;
}

/**
 * Print how this program should be run.
 */
void print_usage(char **argv)
{
	printf("\x1b[1;31mUSAGE\x1b[0m: %s hostaddress [i/e]\n", argv[0]);
	printf("\thostaddress can be \"localhost\" ");
	printf("and will convert to 127.0.0.1\n");
}
