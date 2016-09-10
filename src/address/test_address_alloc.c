#include <stdio.h>
#include <stdlib.h>

#include "address_alloc.h"

void test_class(address_alloc_ptr addresses, char class);

int main(void) 
{
	address_alloc_ptr addresses = new_address_allocator();
	unsigned char address[4];

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

	printf("\n");
	address[0] = (unsigned char)121;
	address[1] = (unsigned char)254;
	address[2] = (unsigned char)123;
	address[3] = (unsigned char)111;
	print_address(address);
	if (is_private_address(address)) {
		printf("private\n");
	} else {
		printf("public\n");
	}

	address[0] = (unsigned char)10;
	address[1] = (unsigned char)254;
	address[2] = (unsigned char)123;
	address[3] = (unsigned char)111;
	print_address(address);
	if (is_private_address(address)) {
		printf("private\n");
	} else {
		printf("public\n");
	}

	address[0] = (unsigned char)172;
	address[1] = (unsigned char)23;
	address[2] = (unsigned char)123;
	address[3] = (unsigned char)111;
	print_address(address);
	if (is_private_address(address)) {
		printf("private\n");
	} else {
		printf("public\n");
	}

	address[0] = (unsigned char)192;
	address[1] = (unsigned char)168;
	address[2] = (unsigned char)123;
	address[3] = (unsigned char)111;
	print_address(address);
	if (is_private_address(address)) {
		printf("private\n");
	} else {
		printf("public\n");
	}


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
			if (is_private_address(address)) {
			} else {
				printf("That was somehow a public address\n");
			}
		} else {
			printf("error allocating address\n");
		}
		free(address);
		address = NULL;
	}
	
}

