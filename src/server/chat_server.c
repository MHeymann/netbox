#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "users.h"
#include "server_listener.h"
#include "server_speaker.h"

char ch = '\0';
int ip_timeout = 600;
unsigned char serv_ip[4];
unsigned char default_ip[4] = {
	1,
	2,
	3,
	4
};

/*** Helper Function Prototypes ******************************************/

void read_line(FILE *f, char *line);
void get_args(int argc, char *argv[]);
void set_defaults();

/*** The Main Routine ****************************************************/

int main (int argc, char *argv[]) 
{
	pthread_t listen_thread;
	pthread_t speak_thread;
	users_t *users = NULL;
	server_speaker_t *speaker;
	server_listener_t *listener;
	/*
	char *end_ptr;
	char *next_ptr;
	int i;
	*/
	int *ports;
	char line[100];

	set_defaults();

	get_args(argc, argv);	

	printf("Server IP: %d.%d.%d.%d\n",
					serv_ip[0],
					serv_ip[1],
					serv_ip[2],
					serv_ip[3]
					);

	ports = malloc(2 * sizeof(int));
	ports[0] = 8001;
	ports[1] = 8002;

	printf("Starting up server\n");

	/* users is a hashset structure for keeping track of connected users */
	/* it maps socket file descriptors to usernames and vice versa */
	users = new_users();
	
	/* data structures for the threads that listen for incomming data */
	/* and connections and sends out data to the various different users */
	speaker = new_server_speaker(users, serv_ip);
	listener = new_server_listener(ports, 2, users, speaker);

	printf("Using ip timeout period of %d seconds\n", ip_timeout);
	speaker->ip_timeout = ip_timeout;
	listener->ip_timeout = ip_timeout;

	/* Launch the two threads */
	/* args are: the thread, unused attribute, start function, and argument for
	 * start function */
	pthread_create(&listen_thread, NULL, listener_run, (void *)listener);
	pthread_create(&speak_thread, NULL, speaker_run, (void *)speaker);

	printf("Server running\n");

	/* handle input */
	while(TRUE) {
		read_line(stdin, line);
		if (strcmp(line, "quit") == 0) {
			break;
		} else if(strcmp(line, "exit") == 0) {
			break;
		} else if(strcmp(line, "status") == 0) {
			printf("Server running\n");
		} else {
			if (ch == EOF) {
				printf("exit\n");
				break;
			} else {
				printf("Didn't get that!\n");
				printf("you said %s\n", line);
			}
		}
	}

	/* shut down server */
	/* signal stop to threads to break out of while loops*/
	listener_stop(listener);
	speaker_stop(speaker);
	/* join(stop) threads */
	printf("Joining speaker\n");
	pthread_join(speak_thread, NULL);
	printf("Joined listener\n");
	printf("Joining listener\n");
	pthread_join(listen_thread, NULL);
	printf("Joined listener\n");

	/* Free all datastructures */
	free_users(users);
	users = NULL;
	server_listener_free(listener);
	listener = NULL;
	server_speaker_free(speaker);
	speaker = NULL;

	free(ports);

	return 0;
}

/*** Helper Functions ****************************************************/

/* A simple scanner function, so that lines with more than one word 
 * can be read */
void read_line(FILE *f, char *line)
{
	int i;
	for (i = 0; TRUE; i++) {
		ch = (char)fgetc(f);
		switch (ch) {
			case EOF:
				/* fall through */
			case '\n':
				line[i] = '\0';
				return;
			default:
				line[i] = ch;
		}
	}
}

void get_args(int argc, char *argv[])
{
	int i, j;
	char *next_ptr;
	char *end_ptr;
	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-ip=", 4) == 0) {
			next_ptr = argv[i] + 4;
			for (j = 0; j < 4; j++) {
				serv_ip[j] = strtol(next_ptr, &end_ptr, 10);
				if (end_ptr == next_ptr) {
	
					printf("invalid ip address passed as argument\n");
					goto DEFAULT_IP;
				} else  {
					if (j != 3) {
						next_ptr = end_ptr + 1;
					}
				}
			}
			
			continue;
DEFAULT_IP:
			printf("defaulting to %d.%d.%d.%d\n", 
					default_ip[0],
					default_ip[1],
					default_ip[2],
					default_ip[3]
					);
			for (j = 0; j < 4; j++) {
				serv_ip[j] = default_ip[j];
			}
		} else if (strncmp(argv[i], "--timeout=", 10) == 0) {
			next_ptr = argv[i] + 10;
			j = strtol(next_ptr, &end_ptr, 10);
			if (end_ptr == next_ptr) {
				printf("invalid timeout provided.  Using default value\n");
			} else {
				ip_timeout = j;
			}

		} else {
			printf("argument '%s; not recognized\n", argv[i]);
		}
	}
}

void set_defaults()
{
	int i;
	ip_timeout = 600;
	for (i = 0; i < 4; i++) {
		serv_ip[i] = default_ip[i];
	}
}
