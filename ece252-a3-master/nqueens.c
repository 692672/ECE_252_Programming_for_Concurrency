// from the Cilk manual: http://supertech.csail.mit.edu/cilk/manual-5.4.6.pdf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct thread_args {
    char * config;
    int n;
    int i;
};

pthread_mutex_t count_mutex;

int safe(char * config, int i, int j)
{
    int r, s;

    for (r = 0; r < i; r++)
    {
        s = config[r];
        if (j == s || i-r==j-s || i-r==s-j)
            return 0;
    }
    return 1;
}

int count = 0;

void nqueens(char *config, int n, int i)
{
    char * new_config;
    int j;

	pthread_mutex_lock(&count_mutex);
    if (i==n)
    {
        count++;
    }
	pthread_mutex_unlock(&count_mutex);
    
    /* try each possible position for queen <i> */
    for (j=0; j<n; j++)
    {
        /* allocate a temporary array and copy the config into it */
        new_config = malloc((i+1)*sizeof(char));
        memcpy(new_config, config, i*sizeof(char));
        if (safe(new_config, i, j))
        {
            new_config[i] = j;
	        nqueens(new_config, n, i+1);
        }
        free(new_config);
    }
    return;
}

void * start_nqueens(void * args) {
    struct thread_args * p_in = args;
    nqueens(p_in->config, p_in->n, p_in->i);
    return;
}

int main(int argc, char *argv[])
{
    int n;

    if (argc < 2)
    {
        printf("%s: number of queens required\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    struct thread_args in_params[n];
    pthread_t * p_tids = (pthread_t *)malloc(sizeof(pthread_t) * n);
    pthread_mutex_init(&count_mutex, NULL);
    
    printf("running queens %d\n", n);
    
    for (int k = 0; k < n; k++) {
        in_params[k].n = n;
        in_params[k].i = 1;
        in_params[k].config = (char*)malloc(n * sizeof(char));

        memset(in_params[k].config, 0, n * sizeof(char));
        in_params[k].config[0] = k;

	    pthread_create(p_tids + k, NULL, start_nqueens, in_params + k);
    }
    for (int k = 0; k < n; k++) {
        pthread_join(p_tids[k], NULL);
        free(in_params[k].config);
    }
    printf("# solutions: %d\n", count);

    free(p_tids);
    return 0;
}
