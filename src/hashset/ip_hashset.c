/*
 * This lookuptable is specifically for the implementing a* with the aim
 * of solving an n-puzzle.
 *
 * It uses a generic hashtable, based on what Willem Bester made us use in
 * Computer Science 244.  
 *
 * “When the hurlyburly's done,
 * When the battle's lost and won.”
 *				― William Shakespeare, Macbeth
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "hashtable.h"
#include "ip_hashset.h"

/*** Lookuptable struct Description **************************************/

typedef struct ip_hashset {
	hashtable_p ht;
} ip_hashset_t;

/*** Helper Function Prototypes ******************************************/

unsigned long hash_ip(void *key, unsigned int size);
int cmp_ips(void *a, void *b);
void s_val2str(void *key, void *val, char *buffer);
void s_dud_free(void *);
unsigned char *ipdup(unsigned char *s);
void *copy_ip_key(void *key);

/*** Functions ***********************************************************/

/**
 * Allocate the datastructures of a ip_hashset and initialise the parameters.
 *
 * @param[in][out] ht a pointer to a pointer of the newly created ip_hashset.
 */
void ip_hashset_init_defaults(ip_hashset_ptr *hs)
{
	ip_hashset_init(hs, -1, -1);
}	

/**
 * Allocate the datastructures of a ip_hashset and initialise the parameters.
 *
 * @param[in][out] ht		a pointer to a pointer of the newly created 
 *							ip_hashset.
 * @param[in] init_delta	The power to raise two by to get the initial
 *							size of the underlying hashtable.
 * @param[in] delta_diff	The integer number to increment the delta
 *							value by when resizing the underlying 
 *							hashtable.
 */
void ip_hashset_init(ip_hashset_ptr *hs, int init_delta, int delta_diff)
{
	hashtable_p ht = NULL;
	ip_hashset_ptr hset = NULL;

	hset = malloc(sizeof(ip_hashset_t));
	if (!hset) {
		fprintf(stderr, "Error allocating memory for lookuptable.\n");
		*hs = NULL;
		return;
	}

	ht = ht_init(0.75f, init_delta, delta_diff, hash_ip, cmp_ips);
	if (!ht) {
		fprintf(stderr, "Error initializing hashtable.\n");
		free(hset);
		*hs = NULL;
		return;
	}

	hset->ht = ht;

	*hs = hset;
}

/**
 * Insert a string instance into the ip_hashset.
 *
 * @param[in] hs A pointer to the ip_hashset into which to insert.
 *
 * @param[in] skey A pointer to the string to insert.
 *
 * @return SUCCESS(1) if the game was inserted successfully and
 * FAIL(0) if not.
 */
int ip_hashset_insert(ip_hashset_ptr hs, unsigned char *ipkey, int fdvalue)
{
	int insert_status;
	unsigned char *ipcopy = ipdup(ipkey);
	long fdl;
	if (!ipcopy) {
		fprintf(stderr, "failed to copy ip\n");
	}

	fdl = fdvalue;
	insert_status = ht_insert(hs->ht, (void *)ipcopy, (void *)fdl);
	if (insert_status) {
		switch (insert_status) {
			case 1:
				fprintf(stderr, 
						"Memory error when inserting into hashtable\n");
				break;
			case KEY_PRESENT_IN_TABLE:
				/*just means it is present.*/
#ifdef DEBUGHS
				fprintf(stdout, "already in lookuptable\n");
#endif
				break;
			default:
				fprintf(stderr, 
						"This is weird in ip_hashset_insert table's switch statement: %d\n", 
						insert_status);
				break;
		}
		free(ipcopy);
		printf("returning fail %d\n", FAIL);
		return FAIL;
	} else {
		return SUCCESS;
	}
}

void ip_hashset_remove(ip_hashset_ptr hs, unsigned char *key)
{
	ht_remove(hs->ht, (void *)key, free, s_dud_free);
}

int ip_get_fd(ip_hashset_t *shs, unsigned char *ip)
{
	void *value = NULL;
	long fdl;
	if (ht_lookup(shs->ht, (void *)ip, &value)) {
		fdl = (long)value;

		return (int)fdl;
	} else {
		return 0;
	}
}

/**
 * Force insertion of a string and fd instance into the ip_hashset.
 *
 * @param[in] hs A pointer to the ip_hashset into which to insert.
 *
 * @param[in] string A pointer to the game instance to insert.
 *
 * @return SUCCESS(1) if the game was inserted successfully and
 * FAIL(0) if not.
 */
/*
int ip_hashset_force_insert(ip_hashset_ptr hs, game_t *game)
{
	int insert_status;
	state_t *state = (state_t *) game;

	insert_status = ht_force_insert(hs->ht, (void *)state, (void *)game);
	if (insert_status) {
		switch (insert_status) {
			case 1:
				fprintf(stderr, 
						"Memory error when inserting into hashtable\n");
				break;
			default:
				fprintf(stderr, 
						"This is weird in ip_hashset_insert table's switch statement: %d\n", 
						insert_status);
				break;
		}
		return FAIL;
	} else {
		return SUCCESS;
	}
}
*/

/**
 * Get a count of the total number of items in the ip_hashset.
 *
 * @param[in] hs A pointer to the ip_hashset for which to get a count.
 *
 * @return An integer of the total number of items in the ip_hashset.
 */
int ip_hashset_content_count(ip_hashset_ptr hs)
{
	return ht_item_count(hs->ht);
}



/**
 *	Print the underlying hashtable of the ip_hashset. 
 *
 *	@param[in] hs A pointer to the ip_hashset to print.  
 */
void print_ip_hashset(ip_hashset_ptr hs)
{
	print_ht(hs->ht, s_val2str);	
}

/**
 * Free the values in the ip_hashset and the ip_hashset itself. 
 *
 * @param[in] hs The ip_hashset to be free'd.  
 */
void free_ip_hashset(ip_hashset_ptr hs)
{
	ht_free(hs->ht, free, s_dud_free);
	hs->ht = NULL;
	free(hs);
}

queue_t *shs_get_keys(ip_hashset_t *s_hs)
{
	return get_keys(s_hs->ht, copy_ip_key, cmp_ips, free);
}


/*** Helper Functions ****************************************************/

void *copy_ip_key(void *key)
{
	return (void *)ipdup((unsigned char *)key);
}

/**
 * Get a string representation of a key value pair, to be put in buffer.
 *
 * @param[in] key The unique key of the pair, used to hash with.   
 *
 * @param[in] val The value of the pair.
 *
 * @param[out] buffer The char pointer where the string representation 
 * of the pair will be put.  
 */
void ip_val2str(void *key, void *val, char *buffer)
{
	unsigned char *ip = (unsigned char *)key;
	long fd = (long) val;
	struct sockaddr_in address;
	socklen_t addrlen;

	getsockname(fd, (struct sockaddr *)&address, &addrlen);

	sprintf(buffer, "ip: %d.%d.%d.%d, socket fd: \t%ld, ip: \t%s, port:\t%d",
			(int)ip[0],
			(int)ip[1],
			(int)ip[2],
			(int)ip[3], fd, 
			inet_ntoa(address.sin_addr), 
			ntohs(address.sin_port));	
}


/** 
 * Just a dud function, as in our case, the key and the value are the
 * same thing.  
 * The stuff inside are simply to keep the warning flags off.  
 *
 * @param[in] key The key that doesn't need freeing. 
 */
void s_dud_free(void *key) 
{
	if ((long)key & 0) {
		return;
	} else {
		return;
	}
}

/**
 * A hash function to be used when placing games in the hashtable.  
 *
 * @param[in] key The key to be used to hash with.  
 *
 * @param[in] size The size of the table into which to put the 
 * key value pair.  
 *
 * @return The hash value as an unsigned long. 
 */
unsigned long hash_ip(void *key, unsigned int size)
{
	unsigned char *string = (unsigned char *)key;
	int i;
	unsigned long hash = 0;

	for (i = 0; i < 4; i++) {
		hash = (hash << 4) + (long)string[i];
	}
	
	return (hash % size);
}

/**
 * A comparison function used to compare two string when trying 
 * to put a value into the hashtable. 
 *  @param[in] a A char pointer
 *
 *  @param[in] b A char pointer
 *
 *  @return An integer value, 0 if the two strings are identical
 *  less than 0 if a comes before b and more than 0 otherwise. 
 */
int cmp_ips(void *a, void *b) 
{
	unsigned char *A = (unsigned char *)a;
	unsigned char *B = (unsigned char *)b;
	unsigned long A_val;
	unsigned long B_val;
	int i;

	for (i = 0; i < 4; i++) {
		A_val = (A_val << 8) + (unsigned long)A[i];
	}
	for (i = 0; i < 4; i++) {
		B_val = (B_val << 8) + (unsigned long)B[i];
	}
	if (A > B) {
		return 1;
	} else if (A < B) {
		return -1;
	} else {
		return 0;
	}
}


unsigned char *ipdup(unsigned char *s) {
	int i;
	int len = 4;
	unsigned char *scopy = malloc(len);
	for (i = 0; i < len; i++) {
		scopy[i] = s[i];
	}
	return scopy;
}
