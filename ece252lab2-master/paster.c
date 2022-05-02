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
#include "catpng.h"
#include "lab_png.h"
#include "main_write_callback.h"

#define IMG_URL "http://ece252-1.uwaterloo.ca:2520/image?img=1"
#define DUM_URL "https://example.com/"

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288 

int image_number;
int glob_size = -1;
int use = 0;
int seen[50];
int count = 0;
char a[50][20];
int all_seen[50];

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
		//printf("%s\n", token);
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

void * get_from_server( ) {
	RECV_BUF png_buffer;
	CURL * curl_handle = curl_easy_init();
	CURLcode res;
	int label;
	int good_png;
	char url[256];
	memset(url, 0, 256);
	
	recv_buf_init(&png_buffer, BUF_SIZE);

	if (curl_handle) {
		char tmp[2] = {'0', '\0'};
		tmp[0] += image_number;
        sprintf(url, "http://ece252-%d.uwaterloo.ca:2520/image?img=%s", use + 1, tmp);

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
		
		if (seen[label] == 0) {
			DIR* dir = opendir("images");
			if (!dir) {
				mkdir("images", 0777);
			}
			
			char fname[20];
			memset(fname, 0, 20*sizeof(char));
			sprintf(fname, "images/%d.png", label);

			/*FILE* f = fopen(fname, "w+");
			fwrite(png_buffer.png_buf, 1, png_buffer.size, f);
			fclose(f);*/
			write_file(fname, png_buffer.buf, png_buffer.size);
			good_png = is_good_png(fname);

			if (good_png == 0) {
				seen[label] = 1;
				strcpy(a[label], fname);
				count++;
			} else {
				remove(fname);
			}

			closedir(dir);
		}
	}
	recv_buf_cleanup(&png_buffer);
	return NULL;
}

int main( int argc, char** argv ) 
{
	int c;
	int t = 1;
	int n = 1;
	char * str = "option requires an argument";
	char input[3][50];
	double time_spent = 0.0;
	memset(input, 0, sizeof(input));
	struct timespec start, finish;

	/*start timing execution*/
	clock_gettime(CLOCK_MONOTONIC, &start);

	while ((c = getopt (argc, argv, "t:n:")) != -1) {
		switch (c) {
        	case 't':
				t = strtoul(optarg, NULL, 10);
	    		if (t <= 0) {
					fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
					return -1;
				}
				break;
        	case 'n':
            	n = strtoul(optarg, NULL, 10);
				if (n <= 0 || n > 3) {
					fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
					return -1;
				}
				break;
			default:
				return -1;
    	}
	}

	image_number = n;
	pthread_t p_tids[50];
	memset(p_tids, 0, 50*sizeof(pthread_t));
	
	memset(seen, 0, 50*sizeof(int));
	memset(a, 0, sizeof(a));

	for (int i = 0; i < 50; i++) {
		all_seen[i] = 1;
	}

    curl_global_init(CURL_GLOBAL_DEFAULT);

	for (int i = 0; i < t; i++) {
		pthread_create(p_tids + i, NULL, get_from_server, NULL);
	}
	
	while (memcmp(seen, all_seen, sizeof(seen)) != 0) {		
			
		for (int i = 0; i < t; i++) {
			pthread_join(p_tids[i], NULL);
		}
		
		if (memcmp(seen, all_seen, sizeof(seen)) != 0) {
			for (int i = 0; i < t; i++) {
				pthread_create(p_tids + i, NULL, get_from_server, NULL);
    		}
		}
	}

	for (int i = 0; i < 50; i++) {
		cat_helper(a[i]);
	}
	cat_images();
	curl_global_cleanup();
	clock_gettime(CLOCK_MONOTONIC, &finish);
	time_spent = (finish.tv_sec - start.tv_sec);
	time_spent += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    return 0;
}
