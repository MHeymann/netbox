#ifndef ADDRESS_ALLOC_H
#define ADDRESS_ALLOC_H

#define RIGHT_BITS	255
#define MIDDLE_BITS (255 << 8)
#define LEFT_BITS	(255 << 16)
#define A 0
#define B 1
#define C 2
#define A_MAX ((long)RIGHT_BITS + (long)MIDDLE_BITS + (long)LEFT_BITS)
#define B_MAX ((long)RIGHT_BITS + (long)MIDDLE_BITS + (long)(15 << 16))
#define C_MAX ((long)RIGHT_BITS + (long)MIDDLE_BITS)



typedef struct address_alloc *address_alloc_ptr;

unsigned char *allocate_address(address_alloc_ptr);

address_alloc_ptr new_address_allocator();

void print_address(unsigned char *address);

#ifdef DEBUG 

void set_counter(address_alloc_ptr addresses, long count);

void set_class(address_alloc_ptr addresses, char class);

#endif


#endif
