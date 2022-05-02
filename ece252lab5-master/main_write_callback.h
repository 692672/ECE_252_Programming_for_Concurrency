#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct recv_buf {
	char buf[10000];       /* memory to hold a copy of received data */
	size_t size;     /* size of valid data in buf in bytes*/
	size_t max_size; /* max capacity of buf in bytes*/
	int file_index;
	int shm_index;
} RECV_BUF;


size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);
