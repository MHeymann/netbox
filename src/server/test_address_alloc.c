#include <stdio.h>
#include <stdlib.h>

#include "address_alloc.h"

void test_class(address_alloc_ptr addresses, char class);

int main(void) 
{
	address_alloc_ptr addresses = new_address_allocator();

	if(!addresses) {
		printf("not allocated\n");
		return 1;
	}
#ifndef DEBUG 
	free(addresses);
	printf("Please compile testing suite with flag -DDEBUG\n");
	exit(2);
#endif 

	printf("Class A\n");
	test_class(addresses, (char)A);
	printf("Class B\n");
	test_class(addresses, (char)B);
	printf("Class C\n");
	test_class(addresses, (char)C);

	free(addresses);
	return 0;
}

void test_class(address_alloc_ptr addresses, char class)
{
	unsigned char *address = NULL;
	int i;
	set_class(addresses, class);
	switch (class) {
		case A:
			set_counter(addresses, A_MAX - 1);
			break;
		case B:
			set_counter(addresses, B_MAX - 1);
			break;
		case C:
			set_counter(addresses, C_MAX - 1);
			break;
		default:
			return;
	}
	for (i = 0; i < 4; i++) {
		address = allocate_address(addresses);
		if (address) {
			printf("address %d: ", i + 1);
			print_address(address);
		} else {
			printf("error allocating address\n");
		}
		free(address);
		address = NULL;
	}
	
}

