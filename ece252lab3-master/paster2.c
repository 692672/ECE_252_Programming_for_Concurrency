/*
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */

/**
 * @file main_simple.c
 * @brief Using cURL to get request a URL and output the response
 *        from the server to standard output
 * SYNOPSIS
 *      <command> [URL]
 * NOTES: -DDEBUG_1 will show more debugging messages to stderr
 */

#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <dirent.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include "catpng.h"
#include "lab_png.h"
#include "shm_stack.h"

#define IMG_URL "http://ece252-1.uwaterloo.ca:2520/image?img=1"
#define DUM_URL "https://example.com/"
#define SHM_SIZE 200000
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288 

int image_number;
int glob_size = -1;
int use = 0;
int seen[50];
int count = 0;
char a[50][20];
int all_seen[50];
int * indextmp;
int * producer_count;
int * consumer_count;

pthread_mutex_t mutex;
pthread_mutexattr_t attr;

size_t write_cb_function(void * ptr, size_t size, size_t nmemb, void * arg) {
	RECV_BUF * buffStruct = (RECV_BUF *)arg;
	memcpy(buffStruct->buf, ptr, size*nmemb);
	buffStruct->size = size*nmemb;
	glob_size = size*nmemb;
	return size*nmemb;
}

size_t header_callback(char * buffer, size_t size, size_t nitems, void * userdata) {
	const char s[2] = "\n";
	char * token = strtok(buffer, s);
	while(token != NULL) {
		if (strncmp(token, "X-Ece252-Fragment:", 18) == 0) {
			char* token2 = strtok(token, " ");
			while (token2 != NULL) {
				*(int*)userdata = atoi(token2);
				token2 = strtok(NULL, " ");
			}
		}
		token = strtok(NULL, s);
	}
	return nitems * size;
}

void get_from_server(int M, ISTACK * buf) {
	RECV_BUF png_buffer;
	CURL * curl_handle = curl_easy_init();
	CURLcode res;
	PNG_DATA png_data;
	int label;
	int good_png;
	char url[256];
	memset(url, 0, 256);
	
	recv_buf_init(&png_buffer, BUF_SIZE);

	if (curl_handle) {
		char tmp[2] = {'0', '\0'};
		tmp[0] += image_number;
        sprintf(url, "http://ece252-%d.uwaterloo.ca:2530/image?img=1&part=%d", (M % 3) +1, M);
		use = (use + 1) % 3;

		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &label);

		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_function);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &png_buffer);
		
		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			printf("CURLE NOT OK: %d\n", res);
		}
		
		curl_easy_cleanup(curl_handle);
		
		char fname[20];
		memset(fname, 0, 20*sizeof(char));
		sprintf(fname, "images/%d.png", M);

		good_png = is_good_png_buf(png_buffer);
		
		if (good_png == 0) {
			png_data.png_name = (char*)malloc(20*sizeof(char));
			memcpy(png_data.png_name, &fname, 20);
			png_data.png = png_buffer.buf;
			push(buf, png_data);
			printf("png_name = %s, fname = %s\n", png_data.png_name, fname);
			printf("buf size: %d", sizeof(buf));
		}
		/*
		if (good_png == 0) {
			seen[label] = 1;
			strcpy(a[label], fname);
			printf("GOOD PNG\n");
			count++;
		} else {
			remove(fname);
		}

		closedir(dir);*/
	}
	recv_buf_cleanup(&png_buffer);
	return NULL;
}

int producer (ISTACK * buf) {
	int index_local;
	int local_bool = 0;
	
	pthread_mutex_lock(&mutex);
	if (buf->pos < buf->size) {
		local_bool = 1;
		index_local = *indextmp;
		(*indextmp)++;
		printf("Size of Stack: %d\n", buf->pos);
		producer_count++;
	}
	pthread_mutex_unlock(&mutex);
	if (local_bool) {
		get_from_server(index_local, buf);
	}

	return 0;
}

int consumer (ISTACK * buf) {
	int index_local;
	PNG_DATA png_data;
	
	DIR* dir = opendir("images");
	if (!dir) {
		mkdir("images", 0777);
	}
	printf("Opened Dir\n");
	if (buf->pos >= 0) {
		pthread_mutex_lock(&mutex);
		pop(buf, &png_data);
		consumer_count++;
		printf("PNG name: %s\n", png_data.png_name);
		fflush(stdout);
		write_file(png_data.png_name, png_data.png, png_data.size);
		pthread_mutex_unlock(&mutex);
	}
	printf("Size of Stack: %d\n", buf->pos);

	return 0;
}

int main( int argc, char** argv ) 
{
	int B, P, C, X, N;

	pid_t pid = 0;
	B = atoi(argv[1]);
	P = atoi(argv[2]);
	C = atoi(argv[3]);
	X = atoi(argv[4]);
	N = atoi(argv[5]);
	pid_t cpid [C];
	pid_t ppid [P];
	ISTACK * buf;
	int shmid = shmget(IPC_PRIVATE, sizeof_shm_stack(B), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int shmid2 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int shmid3 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int shmid4 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	char input[3][50];
	
	memset(input, 0, sizeof(input));

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&mutex, &attr);

	int status = 0;

	if (shmid == -1) {
		perror("schmget");
	} else {
		buf = (ISTACK *) shmat(shmid, NULL, 0);
		indextmp = (int *) shmat(shmid2, NULL, 0);
		producer_count = (int *) shmat(shmid3, NULL, 0);
		consumer_count = (int *) shmat(shmid4, NULL, 0);
		
		init_shm_stack(buf, B);
		*indextmp = 1;
		*producer_count = 0;
		*consumer_count = 0;

		if (buf == (void *) -1) {
			perror("shmat");
			abort();
		}
	}	
	
	for (int i = 0; i < P; i++) {
		pid = fork();

		if (pid > 0) {
			waitpid(pid, &status, 0);
			ppid[i] = pid;
		} else if (pid == 0) {
			producer(buf);
			exit(0);
		} else {
			perror("fork");
			abort();
		}
	}

	for (int j = 0; j < C; j++) {
		pid = fork();
		
		if (pid > 0) {
			waitpid(pid, &status, 0);
			cpid[j] = pid;
		} else if (pid == 0) {
			consumer(buf);
		} else {
			perror("fork");
			abort();
		}
	}
	
	/*image_number = N;
	memset(seen, 0, 50*sizeof(int));
	memset(a, 0, sizeof(a));

	for (int i = 0; i < 50; i++) {
		all_seen[i] = 1;
	}

    curl_global_init(CURL_GLOBAL_DEFAULT);

	while (memcmp(seen, all_seen, sizeof(seen)) != 0) {		
		kill the producers
	}

	for (int i = 0; i < 50; i++) {
		cat_helper(a[i]);
	}
	cat_images();
	curl_global_cleanup();*/
	pthread_mutex_destroy(&mutex);
	pthread_mutexattr_destroy(&attr);
	return 0;
}
