#ifndef Q3HELPER_H
#define Q3HELPER_H

#include <semaphore.h>

#define NAME_MAX 1024

/* Barrier synchronization type.  Used to implement a reusable barrier.  */
typedef struct {
	sem_t mutex;
	sem_t turnstile1, turnstile2;
	unsigned int count, n;
} barrier_t;

/* Initialize barrier struct. n is the number of processes that will
 * synchronize at this barrier. */
void bInit(barrier_t *b, unsigned int n);

/* Synchronize at the barrier. Reusable.  */
void bSync(barrier_t * b);

/* Link type. */
typedef struct {
	unsigned int src, dst;
} link_t;

/* Read web of links from input file. Returns 1 on success, 0 on failure. */
int readLinks(const char *filename, link_t **links, size_t *n_links,
		char (**names)[1024], size_t *n_pages);

#endif

