#include "lab_png.h"
/* The code is 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */
/**
 * @brief  stack to push/pop integer(s), API header  
 * @author yqhuang@uwaterloo.ca
 */

typedef struct png_specs {
	size_t size;
	size_t max_size;
	char * png_name;
	char * png;
} PNG_DATA;

typedef struct int_stack {
	int size;
	int pos;
	//PNG_DATA * items;
	RECV_BUF *items;
} ISTACK;

int sizeof_shm_stack(int size);
int init_shm_stack(struct int_stack *p, int stack_size);
struct int_stack *create_stack(int size);
void destroy_stack(struct int_stack *p);
int is_full(struct int_stack *p);
int is_empty(struct int_stack *p);
int push(struct int_stack *p, RECV_BUF item);
int pop(struct int_stack *p, RECV_BUF * p_item);
