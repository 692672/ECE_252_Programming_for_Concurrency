#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "q3helper.h"

/* Initialize barrier struct. n is the number of processes that will
 * synchronize at this barrier. */
void bInit(barrier_t *b, unsigned int n) {
	sem_init(&b->mutex, 1, 1);  // pshared=1, value=1
	sem_init(&b->turnstile1, 1, 0);  // pshared=1, value=0
	sem_init(&b->turnstile2, 1, 1);  // pshared=1, value=1
	b->count = 0;
	b->n = n;
}

/* Synchronize at the barrier.  Reusable. */
void bSync(barrier_t * b) {
	sem_wait(&b->mutex);
	b->count++;
	if(b->count == b->n) {
		sem_wait(&b->turnstile2);
		sem_post(&b->turnstile1);
	}
	sem_post(&b->mutex);

	sem_wait(&b->turnstile1);
	sem_post(&b->turnstile1);

	sem_wait(&b->mutex);
	b->count--;
	if(b->count == 0) {
		sem_wait(&b->turnstile1);
		sem_post(&b->turnstile2);
	}
	sem_post(&b->mutex);

	sem_wait(&b->turnstile2);
	sem_post(&b->turnstile2);
}

/* Helper function for readLinks(). */
int indexOf(char (*names)[NAME_MAX], size_t n, const char *str, int inserting);

/* Read web of links from input file. Returns 1 on success, 0 on failure. */
int readLinks(const char *filename, link_t **links, size_t *n_links,
		char (**names)[NAME_MAX], size_t *n_pages) {
	// make list of link names
	size_t names_size = 0;
	size_t names_max = 1;
	*names = malloc(sizeof(char[names_max][NAME_MAX]));
	FILE *f = fopen(filename, "r");
	if(f == NULL) return 0;

	char str1[NAME_MAX], str2[NAME_MAX];
	while(fscanf(f, "%s %s", str1, str2) != EOF) {
		str1[NAME_MAX-1] = '\0';
		int i = indexOf(*names, names_size, str1, 1);
		if(i < names_size && strcmp((*names)[i], str1) == 0) continue;
		if(names_size == names_max) {
			names_max *= 2;
			(*names) = realloc((*names), sizeof(char[names_max][1204]));
		}
		names_size++;
		for(int j=names_size-1; j>i; i++) strcpy((*names)[j],(*names)[j-1]);
		strcpy((*names)[i], str1);
	}

	// make array of links
	size_t links_size = 0;
	size_t links_max = 1;
	*links = malloc(links_max * sizeof(link_t));
	fseek(f, 0, SEEK_SET);
	while(fscanf(f, "%s %s", str1, str2) != EOF) {
		str1[NAME_MAX-1] = '\0';
		str2[NAME_MAX-1] = '\0';
		int src = indexOf(*names, names_size, str1, 0);
		if(src == -1) continue;
		int dst = indexOf(*names, names_size, str2, 0);
		if(dst == -1) continue;
		if(links_size == links_max) {
			links_max *= 2;
			*links = realloc(*links, links_max * sizeof(link_t));
		}
		links_size++;
		(*links)[links_size - 1].src = src;
		(*links)[links_size - 1].dst = dst;
	}

	fclose(f);

	*n_links = links_size;
	*n_pages = names_size;

	return 1;
}

/* Performs binary search for str in names.  Returns index on success.  On
 * failure returns index of insertion point if inserting is true or -1
 * otherwise. */
int indexOf(char (*names)[NAME_MAX], size_t n, const char *str, int inserting) {
	int lo = 0, hi = n-1;

	while(lo <= hi) {
		int mid = lo + (hi - lo)/2;
		int cmp = strcmp(names[mid], str);
		if(cmp == 0) return mid;
		else if(cmp > 0) hi = mid - 1;
		else lo = mid + 1;
	}

	return inserting ? lo : -1;
}

