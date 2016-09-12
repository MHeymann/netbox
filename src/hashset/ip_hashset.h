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
#ifndef IP_HASHSET_H
#define IP_HASHSET_H

#include "../queue/queue.h"


/*** Macros **************************************************************/

#define TRUE	1
#define SUCCESS	1
#define FALSE	0
#define FAIL	0
#define HT_ERROR 5


/*** Type Definitions ****************************************************/

typedef struct ip_hashset *ip_hashset_ptr;

/*** Function Prototypes *************************************************/

/**
 * Allocate the datastructures of a ip_hashset and initialise the parameters.
 *
 * @param[in][out] ht a pointer to a pointer of the newly created ip_hashset.
 */
void ip_hashset_init_defaults(ip_hashset_ptr *hs);


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
void ip_hashset_init(ip_hashset_ptr *hs, int init_delta, int delta_diff);

/**
 * Insert a game instance into the ip_hashset.
 *
 * @param[in] hs   A pointer to the ip_hashset into which to insert.
 *
 * @param[in] game A pointer to the game instance to insert.
 *
 * @return SUCCESS(1) if the game was inserted successfully and
 * FAIL(0) if not.
 */
int ip_hashset_insert(ip_hashset_ptr hs, char *skey, int fdvalue);

int name_get_fd(ip_hashset_ptr shs, char *name);

void ip_hashset_remove(ip_hashset_ptr hs, char *key);

/**
 * Get a count of the total number of items in the ip_hashset.
 *
 * @param[in] hs A pointer to the ip_hashset for which to get a count.
 *
 * @return An integer of the total number of items in the ip_hashset.
 */
int ip_hashset_content_count(ip_hashset_ptr hs);

/**
 *	Print the underlying hashtable of the ip_hashset. 
 *
 *	@param[in] hs A pointer to the ip_hashset to print.  
 */
void print_ip_hashset(ip_hashset_ptr hs);

/**
 * Free the values in the ip_hashset and the ip_hashset itself. 
 *
 * @param[in] hs The ip_hashset to be free'd.  
 */
void free_ip_hashset(ip_hashset_ptr hs);

queue_t *iphs_get_keys(ip_hashset_ptr s_hs);

#endif
