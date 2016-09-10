#include <stdio.h>
#include <stdlib.h>
#include "address_alloc.h"

typedef struct address_alloc {
	char class;
	long counter;
} address_alloc_t;

/*** Helper Function Prototypes ******************************************/


/*** Functions ***********************************************************/

unsigned char *allocate_address(address_alloc_ptr addresses) 
{
	int class;
	unsigned char *ret_address = NULL;
	if (!addresses) {
		return NULL;
	}
	ret_address = malloc(4);
	if (!ret_address) {
		printf("Memory error\n");
		return NULL;
	}

	class = (int)addresses->class;

	switch (class) {
CASE_A:
		case A:
			if (addresses->counter > A_MAX) {
				addresses->class = (addresses->class + 1) % 3;
				addresses->counter = 0;
			} else {
				ret_address[0] = (unsigned char)10;
				ret_address[1] = 
					(unsigned char)((LEFT_BITS & addresses->counter) >> 16);
				ret_address[2] = 
					(unsigned char)((MIDDLE_BITS & addresses->counter) >> 8);
				ret_address[3] = 
					(unsigned char)(RIGHT_BITS & addresses->counter);
				break;
			}
			/* Fall through */
		case B:
			if (addresses->counter > B_MAX) {
				addresses->class = (addresses->class + 1) % 3;
				addresses->counter = 0;
			} else {
				ret_address[0] = (unsigned char)172;
				ret_address[1] = 
					(unsigned char)(16 + ((LEFT_BITS & addresses->counter) >> 16));
				ret_address[2] = 
					(unsigned char)((MIDDLE_BITS & addresses->counter) >> 8);
				ret_address[3] = 
					(unsigned char)(RIGHT_BITS & addresses->counter);
				break;
			}
			/* Fall through */
		case C:
			if (addresses->counter > C_MAX) {
				addresses->class = (addresses->class + 1) % 3;
				addresses->counter = 0;
				goto CASE_A;
			} else {
				ret_address[0] = (unsigned char)192;
				ret_address[1] = (unsigned char)168;
				ret_address[2] = 
					(unsigned char)((MIDDLE_BITS & addresses->counter) >> 8);
				ret_address[3] = 
					(unsigned char)(RIGHT_BITS & addresses->counter);
				break;
			}
		default:
			printf("big fault in updating class number\n");
	}
	addresses->counter++;
	return ret_address;
}


address_alloc_t *new_address_allocator() 
{
	address_alloc_t *addresses = malloc(sizeof(address_alloc_t));

	if (addresses) {
		addresses->class = (char)0;
		addresses->counter = 0;
	}

	return addresses;
}

void print_address(unsigned char *address)
{
	int i;
	if (!address) {
		return;
	}
	printf("%d", (int)address[0]);
	for (i = 1; i < 4; i++) {
		printf(".%d", (int)address[i]);
	}
	printf("\n");
}

#ifdef DEBUG

void set_counter(address_alloc_ptr addresses, long count) 
{
	addresses->counter = count;
}

void set_class(address_alloc_ptr addresses, char class) 
{
	addresses->class = class;
}

#endif

/*** Helper Functions ****************************************************/
