/*
 * The code is derived from cURL example and paster.c base code.
 * The cURL example is at URL:
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * Copyright (C) 1998 - 2018, Daniel Stenberg, <daniel@haxx.se>, et al..
 *
 * The xml example code is 
 * http://www.xmlsoft.org/tutorial/ape.html
 *
 * The paster.c code is 
 * Copyright 2013 Patrick Lam, <p23lam@uwaterloo.ca>.
 *
 * Modifications to the code are
 * Copyright 2018-2019, Yiqing Huang, <yqhuang@uwaterloo.ca>.
 * 
 * This software may be freely redistributed under the terms of the X11 license.
 */

/** 
 * @file main_wirte_read_cb.c
 * @brief cURL write call back to save received data in a user defined memory first
 *        and then write the data to a file for verification purpose.
 *        cURL header call back extracts data sequence number from header if there is a sequence number.
 * @see https://curl.haxx.se/libcurl/c/getinmemory.html
 * @see https://curl.haxx.se/libcurl/using/
 * @see https://ec.haxx.se/callback-write.html
 */ 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#define _GNU_SOURCE
#include <search.h>
#include <pthread.h>
#include <semaphore.h>

#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */

#define CT_PNG  "image/png"
#define CT_HTML "text/html"
#define CT_PNG_LEN  9
#define CT_HTML_LEN 9
#define MAX_SIZE 10000

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/*struct hsearch_data* hash_table;
char* glob_href;*/

char visited_urls[10000][500];
int vu_ind = 0;
int vu_size = 1;

int png_count = 0;

FILE* fptr1;

pthread_t* p_tids;

int c;
int t = 1;
int m = 50;
char * v = "";
char * str = "option requires an argument";
char init_url[256];

pthread_mutex_t mutex;

int idle = 0;

pthread_cond_t all_idle;

sem_t s_wait;

typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;


htmlDocPtr mem_getdoc(char *buf, int size, const char *url);
xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath);
int find_http(char *fname, int size, int follow_relative_links, const char *base_url);
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
void cleanup(CURL *curl, RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);
CURL *easy_handle_init(RECV_BUF *ptr, const char *url);
int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf);

int get_vu_ind() {
	int val;
	pthread_mutex_lock(&mutex);
	val = vu_ind;
	pthread_mutex_unlock(&mutex);
	return val;
}

int get_vu_size() {
	int val;
	pthread_mutex_lock(&mutex);
	val = vu_size;
	pthread_mutex_unlock(&mutex);
	return val;
}

int get_png_count() {
	int val;
	pthread_mutex_lock(&mutex);
	val = png_count;
	pthread_mutex_unlock(&mutex);
	return val;
}

int is_png(unsigned char * buf, size_t n) {
    for (int i = 0; i < n; i++) {
        if (i == 0 && buf[i] != 0x89) {
            return 0;
        } else if (i == 1 && buf[i] != 0x50) {
            return 0;
        } else if (i == 2 && buf[i] != 0x4E) {
            return 0;
        } else if (i == 3 && buf[i] != 0x47) {
            return 0;
        } else if (i == 4 && buf[i] != 0x0D) {
            return 0;
        } else if (i == 5 && buf[i] != 0x0A) {
            return 0;
        } else if (i == 6 && buf[i] != 0x1A) {
            return 0;
        } else if (i == 7 && buf[i] != 0x0A) {
            return 0;
        }
    }
    return 1;
}

htmlDocPtr mem_getdoc(char *buf, int size, const char *url)
{
    int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | \
               HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    htmlDocPtr doc = htmlReadMemory(buf, size, url, NULL, opts);
    
    if ( doc == NULL ) {
        //fprintf(stderr, "Document not parsed successfully.\n");
        return NULL;
    }
    return doc;
}

xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath)
{
	
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context == NULL) {
        //printf("Error in xmlXPathNewContext\n");
        return NULL;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (result == NULL) {
        //printf("Error in xmlXPathEvalExpression\n");
        return NULL;
    }
    if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
        xmlXPathFreeObject(result);
        //printf("No result\n");
        return NULL;
    }
    return result;
}

int find_http(char *buf, int size, int follow_relative_links, const char *base_url)
{

    int i;
    htmlDocPtr doc;
    xmlChar *xpath = (xmlChar*) "//a/@href";
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result;
    xmlChar *href;
		
    if (buf == NULL) {
        return 1;
    }

    doc = mem_getdoc(buf, size, base_url);
    result = getnodeset (doc, xpath);
    if (result) {
        nodeset = result->nodesetval;
        for (i=0; i < nodeset->nodeNr; i++) {
            href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
            if ( follow_relative_links ) {
                xmlChar *old = href;
                href = xmlBuildURI(href, (xmlChar *) base_url);
                xmlFree(old);
            }
            if ( href != NULL && !strncmp((const char *)href, "http", 4) ) {
                
				pthread_mutex_lock(&mutex);
				int unique = 1;
				for (int i = 0; i < vu_size; i++) {
					if (strcmp(visited_urls[i], (char *)href) == 0) {
						unique = 0;
						break;
					}
				}
				
				if (strcmp((char *)href, "") == 0) {
					unique = 0;
				}

				if (unique == 0) {
					xmlFree(href);
					pthread_mutex_unlock(&mutex);
					continue;
				}

				//printf("adding url: %s\n", href);
				memcpy(visited_urls[vu_size], href, strlen(href));
				vu_size++;
				pthread_mutex_unlock(&mutex);

				sem_post(&s_wait);
				//printf("href: %s\n", href);
				//strcpy(glob_href, href);

				/*ENTRY element, *temp_element;
				element.key = href;
				element.data = "asdfasdfasdf";
				
				hsearch_r(element, ENTER, &temp_element, hash_table);

				ENTRY looking;
      			looking.key = href;

      			ENTRY* found;

      			int fn_res = hsearch_r(looking, FIND, &found, hash_table);*/
            }
            xmlFree(href);
        }
        xmlXPathFreeObject (result);
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
}
/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;

#ifdef DEBUG1_
    //printf("%s", p_recv);
#endif /* DEBUG1_ */
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}


int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be positive */
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}

void cleanup(CURL *curl, RECV_BUF *ptr)
{
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    recv_buf_cleanup(ptr);
}
/**
 * @brief output data in memory to a file
 * @param path const char *, output file path
 * @param in  void *, input data to be written to the file
 * @param len size_t, length of the input data in bytes
 */

int write_file(const char *path, const void *in, size_t len)
{
    FILE *fp = NULL;

    if (path == NULL) {
        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
        fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
}

/**
 * @brief create a curl easy handle and set the options.
 * @param RECV_BUF *ptr points to user data needed by the curl write call back function
 * @param const char *url is the target url to fetch resoruce
 * @return a valid CURL * handle upon sucess; NULL otherwise
 * Note: the caller is responsbile for cleaning the returned curl handle
 */

CURL *easy_handle_init(RECV_BUF *ptr, const char *url)
{
    CURL *curl_handle = NULL;

    if ( ptr == NULL || url == NULL) {
        return NULL;
    }

    /* init user defined call back function buffer */
    if ( recv_buf_init(ptr, BUF_SIZE) != 0 ) {
        return NULL;
    }
    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return NULL;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)ptr);

    /* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)ptr);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ece252 lab4 crawler");

    /* follow HTTP 3XX redirects */
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    /* continue to send authentication credentials when following locations */
    curl_easy_setopt(curl_handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
    /* max numbre of redirects to follow sets to 5 */
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
    /* supports all built-in encodings */ 
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");

    /* Max time in seconds that the connection phase to the server to take */
    //curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
    /* Max time in seconds that libcurl transfer operation is allowed to take */
    //curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
    /* Time out for Expect: 100-continue response in milliseconds */
    //curl_easy_setopt(curl_handle, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);

    /* Enable the cookie engine without reading any initial cookies */
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
    /* allow whatever auth the proxy speaks */
    curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    /* allow whatever auth the server speaks */
    curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

    return curl_handle;
}

int process_html(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    char fname[256];
    int follow_relative_link = 1;
    char *url = NULL; 
    pid_t pid =getpid();

    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
    find_http(p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url); 
    sprintf(fname, "./output_%d.html", pid);
    //return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
	return 0;
}

int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    pid_t pid =getpid();
    char fname[256];
    char *eurl = NULL;          /* effective URL */
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);
    if ( eurl != NULL) {
        //printf("The PNG url is: %s\n", eurl);
    }
	
	//REMOVE
	if (is_png((unsigned char *)p_recv_buf->buf, 8)) {
		//printf("Real PNG\n");
		//fflush(stdout);
		pthread_mutex_lock(&mutex);
		png_count++;
		fprintf(fptr1, "%s\n", eurl);
		pthread_mutex_unlock(&mutex);
	}
	else {
		//printf("Not a real PNG\n");
		//fflush(stdout);
		return 0;
	}

    sprintf(fname, "./output_%d_%d.png", p_recv_buf->seq, pid);
    //return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
	return 0;
}
/**
 * @brief process teh download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */

int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    CURLcode res;
    char fname[256];
    pid_t pid =getpid();
    long response_code;

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if ( res == CURLE_OK ) {
	    //printf("Response code: %ld\n", response_code);
    }

    if ( response_code >= 400 ) { 
    	//fprintf(stderr, "Error.\n");
        return 1;
    }

    char *ct = NULL;
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
    if ( res == CURLE_OK && ct != NULL ) {
    	//printf("Content-Type: %s, len=%ld\n", ct, strlen(ct));
    } else {
        //fprintf(stderr, "Failed obtain Content-Type\n");
        return 2;
    }

    if ( strstr(ct, CT_HTML) ) {
        return process_html(curl_handle, p_recv_buf);
    } else if ( strstr(ct, CT_PNG) ) {
        return process_png(curl_handle, p_recv_buf);
    } else {
        sprintf(fname, "./output_%d", pid);
    }

    return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
}

void * crawler(void* arg) {
	CURL *curl_handle;
    CURLcode res;
    char url[256];
	memcpy(url, init_url, 256);
    RECV_BUF recv_buf;
		
	//while (get_vu_ind() < get_vu_size() && get_png_count() < m) {
	//while (get_vu_ind() < get_vu_size()) {
	while (1) {
      //printf("working on url: %s\n", visited_urls[vu_ind]);

      /*if (strcmp(visited_urls[vu_ind], "") == 0) {
          printf("entered if\n");
          vu_ind++;
          continue;
      }
      else {
          printf("did not enter if\n");
      }
      fflush(stdout);*/
      if (get_png_count() >= m) {
	    pthread_mutex_lock(&mutex);
		pthread_cond_signal(&all_idle);
		pthread_mutex_unlock(&mutex);
		break;
	  }
	  
	  pthread_mutex_lock(&mutex);
	  idle += 1;
	  if (vu_ind >= vu_size && idle == t) {
		pthread_cond_signal(&all_idle);
		pthread_mutex_unlock(&mutex);
		break;
	  }
	  pthread_mutex_unlock(&mutex);
	  
	  sem_wait(&s_wait);
	  
	  idle -= 1;
      int loc_vu_ind;
	  pthread_mutex_lock(&mutex);
	  loc_vu_ind = vu_ind;
	  vu_ind++;
	  pthread_mutex_unlock(&mutex);
	  curl_handle = easy_handle_init(&recv_buf, visited_urls[loc_vu_ind]);

      if ( curl_handle == NULL ) {
          //fprintf(stderr, "Curl initialization failed. Exiting...\n");
          curl_global_cleanup();
          abort();
      }
      /* get it! */
      res = curl_easy_perform(curl_handle);

      if( res != CURLE_OK) {
          //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
          cleanup(curl_handle, &recv_buf);
          //exit(1);
          continue;
      } else {
      /*printf("%lu bytes received in memory %p, seq=%d.\n", \
                 recv_buf.size, recv_buf.buf, recv_buf.seq);*/
      }

      /* process the download data */
      process_data(curl_handle, &recv_buf);

      /* cleaning up */
      cleanup(curl_handle, &recv_buf);

      //printf("ind %d\n", loc_vu_ind);
    }

	return 0;
}

int main( int argc, char** argv ) 
{
    /*CURL *curl_handle;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;*/
    /*hash_table = malloc(MAX_SIZE*1000);
	memset(hash_table, 0, MAX_SIZE*1000);
	hcreate_r(MAX_SIZE, hash_table);
	
	glob_href = malloc(100);*/
	
	xmlInitParser();	
		
	int num_args = 1;
	double times[2];
	struct timeval tv;
	
	memset(visited_urls, 0, 10000*500);

	sem_init(&s_wait, 0, 1);
	pthread_mutex_init(&mutex, NULL);

	/*int c;
    int t = 1;
    int m = 0;
	char * v;
	char * str = "option requires an argument";*/

	if(gettimeofday(&tv, NULL) != 0) {
		perror("gettimeofdat");
		abort();
	}

	times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    if (argc == 1) {
		char * tmp = SEED_URL; 
        strcpy(init_url, tmp); 
    } else {
		char * tmp = argv[1];
        strcpy(init_url, tmp);
    }

	while ((c = getopt (argc, argv, "t:m:v:")) != -1) {
        switch (c) {
            case 't':
                t = strtoul(optarg, NULL, 10);
				num_args += 2;
                if (t <= 0) {
                    fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                    return -1;
                }
                break;
            case 'm':
                m = strtoul(optarg, NULL, 10);
				num_args += 2;
                if (m <= 0 || m > 100) {
                    fprintf(stderr, "%s: %s greater than 0 less than 100  -- 'm'\n", argv[0], str);
                    return -1;
                }
                break;
			case 'v':
                v = optarg;
				num_args += 2;
				if (strcmp(v,"") == 0) {
					fprintf(stderr, "%s: %s does not have a file'n'\n", argv[0], str);
                    return -1;
				}
				break;
			default:
				return -1;
		}
	}

	if (num_args < argc) {
		memcpy(init_url, argv[argc - 1], strlen(argv[argc-1]) + 1);
	} else {
		memcpy(init_url, SEED_URL, strlen(SEED_URL));
	}
	//printf("URL: %s\n", init_url);

    curl_global_init(CURL_GLOBAL_DEFAULT);

	memcpy(visited_urls[vu_ind], init_url, 500);

	fptr1 = fopen("png_urls.txt", "w");

	p_tids = malloc(sizeof(pthread_t)*t);

	for (int i = 0; i < t; i++) {
		pthread_create(p_tids + i, NULL, crawler, NULL);
	}
	
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&all_idle, &mutex);
	pthread_mutex_unlock(&mutex);

	for (int i = 0; i < t; i++) {
		pthread_cancel(p_tids[i]);
	}

	for (int i = 0; i < t; i++) {
		pthread_join(p_tids[i], NULL);
	}

	/*while (vu_ind < vu_size && png_count < m) {

	printf("working on url: %s\n", visited_urls[vu_ind]);
	
	if (strcmp(visited_urls[vu_ind], "") == 0) {
		printf("entered if\n");
		vu_ind++;
		continue;
	}
	else {
		printf("did not enter if\n");
	}
	fflush(stdout);

	curl_handle = easy_handle_init(&recv_buf, visited_urls[vu_ind]);
	vu_ind++;

    if ( curl_handle == NULL ) {
        fprintf(stderr, "Curl initialization failed. Exiting...\n");
        curl_global_cleanup();
        abort();
    }*/
    /* get it! */
    /*res = curl_easy_perform(curl_handle);

    if( res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        cleanup(curl_handle, &recv_buf);
        //exit(1);
		continue;
    } else {
	printf("%lu bytes received in memory %p, seq=%d.\n", \
               recv_buf.size, recv_buf.buf, recv_buf.seq);
    }*/

    /* process the download data */
    //process_data(curl_handle, &recv_buf);
	
    /* cleaning up */
	/*cleanup(curl_handle, &recv_buf);

	printf("ind %d, size: %d, png_count: %d\n", vu_ind, vu_size, png_count);
	}*/

	//printf("ind: %d, size: %d, png_count: %d\n", vu_ind, vu_size, png_count);

	curl_global_cleanup();

	if (strcmp(v,"") != 0) {
		FILE* fptr2 = fopen(v, "w");
		for (int i = 0; i < vu_ind; i++) {
			fprintf(fptr2, "%s\n", visited_urls[i]);
		}
		fclose(fptr2);
	}
	
	pthread_mutex_destroy(&mutex);
	sem_destroy(&s_wait);
	fclose(fptr1);
	//hdestroy_r(hash_table);
	free(p_tids);
    //free(glob_href);
	xmlCleanupParser();

	if (gettimeofday(&tv, NULL) != 0) {
		perror("gettimeofday");
		abort();
	}

	times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
	printf("findpng2 execution time: %.6lf seconds\n", times[1] - times[0]);
	return 0;
}
