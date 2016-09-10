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
#ifndef M_HASHSET_H
#define M_HASHSET_H

#include "../queue/queue.h"


/*** Macros **************************************************************/

#define TRUE	1
#define SUCCESS	1
#define FALSE	0
#define FAIL	0
#define HT_ERROR 5


/*** Type Definitions ****************************************************/

typedef struct mac_hashset *mac_hashset_ptr;

/*** Function Prototypes *************************************************/

/**
 * Allocate the datastructures of a mac_hashset and initialise the parameters.
 *
 * @param[in][out] ht a pointer to a pointer of the newly created mac_hashset.
 */
void mac_hashset_init_defaults(mac_hashset_ptr *hs);


/**
 * Allocate the datastructures of a mac_hashset and initialise the parameters.
 *
 * @param[in][out] ht		a pointer to a pointer of the newly created 
 *							mac_hashset.
 * @param[in] init_delta	The power to raise two by to get the initial
 *							size of the underlying hashtable.
 * @param[in] delta_diff	The integer number to increment the delta
 *							value by when resizing the underlying 
 *							hashtable.
 */
void mac_hashset_init(mac_hashset_ptr *hs, int init_delta, int delta_diff);

/**
 * Insert a game instance into the mac_hashset.
 *
 * @param[in] hs   A pointer to the mac_hashset into which to insert.
 *
 * @param[in] game A pointer to the game instance to insert.
 *
 * @return SUCCESS(1) if the game was inserted successfully and
 * FAIL(0) if not.
 */
int mac_hashset_insert(mac_hashset_ptr hs, unsigned char *skey, int fdvalue);

int mac_get_fd(mac_hashset_ptr shs, unsigned char *name);

void mac_hashset_remove(mac_hashset_ptr hs, unsigned char *key);

/**
 * Get a count of the total number of items in the mac_hashset.
 *
 * @param[in] hs A pointer to the mac_hashset for which to get a count.
 *
 * @return An integer of the total number of items in the mac_hashset.
 */
int mac_hashset_content_count(mac_hashset_ptr hs);

/**
 *	Print the underlying hashtable of the mac_hashset. 
 *
 *	@param[in] hs A pointer to the mac_hashset to print.  
 */
void print_mac_hashset(mac_hashset_ptr hs);

/**
 * Free the values in the mac_hashset and the mac_hashset itself. 
 *
 * @param[in] hs The mac_hashset to be free'd.  
 */
void free_mac_hashset(mac_hashset_ptr hs);

queue_t *mhs_get_keys(mac_hashset_ptr s_hs);

#endif
