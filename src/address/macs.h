#ifndef MACS_H
#define MACS_H

#include <pthread.h>
#include "../hashset/mac_hashset.h"

typedef struct mac_list {
	pthread_mutex_t *lockq;
	mac_hashset_ptr hs;
} mac_list_t;

mac_list_t *new_mac_list();

void free_mac_list(mac_list_t *list);

unsigned char *gen_mac(mac_list_t *list);

#endif
