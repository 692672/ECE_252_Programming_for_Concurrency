/*
 * The code is derived from 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
so we are guaranteed to find the ans within 5% *
 * This software may be freely redistributed under the terms of the X11 License.
 */

/**
 * @brief  stack to push/pop integers.   
 */

#include <stdio.h>
#include <stdlib.h>
#include "shm_stack.h"
#include <sys/shm.h>
#include <sys/stat.h>

/* a stack that can hold integers */
/* Note this structure can be used by shared memory,
   since the items field points to the memory right after it.
   Hence the structure and the data items it holds are in one
   continuous chunk of memory.

   The memory layout:
   +===============+
   | size          | 4 bytes
   +---------------+
   | pos           | 4 bytes
   +---------------+
   | items         | 8 bytes
   +---------------+
   | items[0]      | 4 bytes
   +---------------+
   | items[1]      | 4 bytes
   +---------------+
   | ...           | 4 bytes
   +---------------+
   | items[size-1] | 4 bytes
   +===============+
*/

/**
 * @brief calculate the total memory that the struct int_stack needs and
 *        the items[size] needs.
 * @param int size maximum number of integers the stack can hold
 * @return return the sum of ISTACK size and the size of the data that
 *         items points to.
 */

int sizeof_shm_stack(int size)
{
    return (sizeof(ISTACK) + sizeof(RECV_BUF) * size);
}

/**
 * @brief initialize the ISTACK member fields.
 * @param ISTACK *p points to the starting addr. of an ISTACK struct
 * @param int stack_size max. number of items the stack can hold
 * @return 0 on success; non-zero on failure
 * NOTE:
 * The caller first calls sizeof_shm_stack() to allocate enough memory;
 * then calls the init_shm_stack to initialize the struct
 */
int init_shm_stack(ISTACK *p, int stack_size)
{
    if ( p == NULL || stack_size == 0 ) {
        return 1;
    }

    p->size = stack_size;
    p->pos  = -1;
    p->items = (RECV_BUF *) (p + sizeof(ISTACK));
    return 0;
}

/**
 * @brief create a stack to hold size number of integers and its associated
 *      ISTACK data structure. Put everything in one continous chunk of memory.
 * @param int size maximum number of integers the stack can hold
 * @return NULL if size is 0 or malloc fails
 */

ISTACK *create_stack(int size)
{
    int mem_size = 0;
    ISTACK *pstack = NULL;
    
    if ( size == 0 ) {
        return NULL;
    }

    mem_size = sizeof_shm_stack(size);
    pstack = (ISTACK*) malloc(mem_size);

    if ( pstack == NULL ) {
        perror("malloc");
    } else {
        char *p = (char *)pstack;
        pstack->items = (RECV_BUF *) (p + sizeof(ISTACK));
        pstack->size = size;
        pstack->pos  = -1;
    }

    return pstack;
}

/**
 * @brief release the memory
 * @param ISTACK *p the address of the ISTACK data structure
 */

void destroy_stack(ISTACK *p)
{
    if ( p != NULL ) {
        free(p);
    }
}

/**
 * @brief check if the stack is full
 * @param ISTACK *p the address of the ISTACK data structure
 * @return non-zero if the stack is full; zero otherwise
 */

int is_full(ISTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == (p->size -1) );
}

/**
 * @brief check if the stack is empty 
 * @param ISTACK *p the address of the ISTACK data structure
 * @return non-zero if the stack is empty; zero otherwise
 */

int is_empty(ISTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == -1 );
}

/**
 * @brief push one integer onto the stack 
 * @param ISTACK *p the address of the ISTACK data structure
 * @param int item the integer to be pushed onto the stack 
 * @return 0 on success; non-zero otherwise
 */
/*
int push(ISTACK *p, RECV_BUF item)
{
    printf("entered push()\n");
	fflush(stdout);
	if ( p == NULL ) {
        return -1;
    }
    if ( !is_full(p) ) {
        
		printf("pos before: %d\n", p->pos);
		++(p->pos);
		printf("pos after: %d\n", p->pos);

		printf("inside push(): %d\n", item.index);
		fflush(stdout);

		printf("also inside push(): %d\n", item.size);
		fflush(stdout);

		//int shmid_png_name = shmget(IPC_PRIVATE, 20, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
		//int shmid_p_png_name = shmget(IPC_PRIVATE, sizeof(int*), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
		
		//p->items[p->pos].png_name = shmat(shmid_png_name, NULL, 0);
		//p->items[p->pos].size = shmat(shmid_p_png_name, NULL, 0);
        //memcpy(p->items[p->pos].png_name, item.png_name, 20);
		//memcpy(p->items[p->pos].size, shmid_png_name, 4);
		
		//printf("shmid: %d\n", shmid_png_name);
		//fflush(stdout);
		//printf("also shmid: %d\n", p->items[p->pos].size);
		//fflush(stdout);
		
		p->items[p->pos].index = item.index;
		p->items[p->pos].size = item.size;
		
		int shmid_buf = shmget(IPC_PRIVATE, item.size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
		p->items[p->pos].size = (char *) shmat(shmid_buf, NULL, 0);
		
		printf("%s\n", item.buf);

		memcpy(&p->items[p->pos].buf, &item.buf, item.size);

		printf("inside push(): items[]: %d\n", p->items[p->pos].index);
		fflush(stdout);

		printf("also inside push(): items[]: %d\n", p->items[p->pos].size);
		fflush(stdout);
        return 0;
    } else {
        printf("its full!!!\n");
		fflush(stdout);
		return -1;
    }
}
*/
/**
 * @brief push one integer onto the stack 
 * @param ISTACK *p the address of the ISTACK data structure
 * @param int *item output parameter to save the integer value 
 *        that pops off the stack 
 * @return 0 on success; non-zero otherwise
 */
/*
int pop(ISTACK *p, RECV_BUF *p_item)
{
    printf("entered the pop function!\n");
	fflush(stdout);
		
	if ( p == NULL ) {
        return -1;
    }

    if ( !is_empty(p) ) {
		printf("not empty!\n");
		fflush(stdout);
		
		printf("==========================\n");
		printf("p->pos: %d\n", p->pos);
		fflush(stdout);
		PNG_DATA tmp = p->items[p->pos];
		char* name = tmp.png_name;
		int strlength = strlen(name);
		printf("able to assign to PNG_DATA and name\n");
		fflush(stdout);
		printf("inside pop(): %s\n", name);
		printf("==========================\n");
		fflush(stdout);
		
	
		printf("inside pop(): %d\n", p->items[p->pos].index);
		fflush(stdout);
		
		printf("also inside pop(): %d\n", p->items[p->pos].size);
		fflush(stdout);
		
		printf("also also inside pop(): %s\n", p->items[p->pos].buf);
		fflush(stdout);

		*p_item = p->items[p->pos];
        (p->pos)--;

        printf("successful return\n");
		fflush(stdout);
		return 0;
    } else {
        return 1;
    }
}
*/
