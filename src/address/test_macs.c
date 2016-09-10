#include <stdio.h>
#include <stdlib.h>

#include "macs.h"

void print_mac(unsigned char *address);

int main(void)
{
	int i;
	unsigned char *address = NULL;
	mac_list_t *list = new_mac_list();

	for (i = 0; i < 10000; i++) {
		printf("%d) ", i + 1);
		address = gen_mac(list);
		print_mac(address);
		address = NULL;
	}

	free_mac_list(list);
	list = NULL;

	return 0;
}


void print_mac(unsigned char *address)
{
	int i;
	if (!address) {
		return;
	}

	if ((int)address[0] < 16) {
		printf("0%x", (unsigned int)address[0]);
	} else {
		printf("%x", (unsigned int)address[0]);
	}
	for (i = 1; i < 6; i++) {
		if ((int)address[i] < 16) {
			printf(":0%x", (unsigned int)address[i]);
		} else {
			printf(":%x", (unsigned int)address[i]);
		}
	}
	printf("\n");

}
