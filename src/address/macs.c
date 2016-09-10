#include <stdio.h>
#include <stdlib.h>

#include "macs.h"

/*
typedef struct mac_list {
	pthread_mutex_t *lockq;
	mac_hashset_ptr q;
} mac_list_t;
*/

void random_mac(unsigned char *mac);
unsigned char random_char();

mac_list_t *new_mac_list()
{
	mac_hashset_ptr hs = NULL;
	mac_list_t *list = malloc(sizeof(mac_list_t));

	if (!list) {
		return NULL;
	}

	list->lockq = malloc(sizeof(pthread_mutex_t));
	mac_hashset_init_defaults(&hs);
	list->hs = hs;

	if (list->lockq) {
		pthread_mutex_init(list->lockq, NULL);
	}
	
	if ((!list->hs) || (!list->lockq)) {
		free_mac_list(list);
		return NULL;
	}

	srand((int)time(NULL));

	return list;
}

void free_mac_list(mac_list_t *list)
{
	if (!list) {
		return;
	}

	if (list->lockq) {
		pthread_mutex_destroy(list->lockq);
		free(list->lockq);
		list->lockq = NULL;
	}
	if (list->hs) {
		free_mac_hashset(list->hs);
		list->hs = NULL;
	}
	
	free(list);
}

unsigned char *gen_mac(mac_list_t *list)
{
	unsigned char *mac = malloc(6);

	random_mac(mac);

	while(!mac_hashset_insert(list->hs, mac, 1)) {
		random_mac(mac);
	}

	/* remember, this will be free'd n mac_hashset */
	return mac;
}


/*** Helper Functions ****************************************************/

void random_mac(unsigned char *mac) 
{
	int i;

	for (i = 0; i < 6; i++) {
		mac[i] = random_char();
	}
}

unsigned char random_char()
{
	unsigned char c;
	int r = rand();

	c = (unsigned char)(r % 256);
	return c;
}

