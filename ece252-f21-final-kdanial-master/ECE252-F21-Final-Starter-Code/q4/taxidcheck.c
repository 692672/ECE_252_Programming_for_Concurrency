#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define TAX_ID_LENGTH 9
#define NUM_THREADS 4

bool check_tax_id (char* tax_id);
char *get_next_tax_id(FILE *inputfile);
void write_output(char *tax_id, bool valid, FILE *outputfile);

pthread_mutex_t count_mutex;
int valid_num;
int invalid_num;

struct thread_args {
	FILE * inputfd;
    FILE * outputfd;
    char * taxid;
	bool validid;
};

void * thread_task(void * args) {
	struct thread_args * p_in = args;
	p_in->taxid = malloc(TAX_ID_LENGTH * sizeof(char));
	/*struct thread_results * p_out = malloc(sizeof(struct thread_results));*/
	
	while (p_in->taxid != NULL){
		p_in->taxid =  get_next_tax_id(p_in->inputfd);
		if (p_in->taxid == NULL)
			continue;
		p_in->validid = check_tax_id(p_in->taxid);
		write_output(p_in->taxid, p_in->validid, p_in->outputfd);

		pthread_mutex_lock(&count_mutex);
		if (p_in->validid == 1) {
			valid_num += 1;
		} else {
			invalid_num += 1;
		}
		pthread_mutex_unlock(&count_mutex);
	}
	/*free(p_out);*/
}

int main( int argc, char** argv ) {
    if (argc != 2) {
        printf("Usage: %s [filename]", argv[0]);
        return -1;
    }

    int* invalid_tax_id_count = (int *)malloc( sizeof(int) );
    *invalid_tax_id_count = 0;
    int* valid_tax_id_count = (int *)malloc( sizeof(int) );
    *valid_tax_id_count = 0;

    FILE *inputfile = fopen(argv[1], "r");
    if (inputfile == NULL) {
        printf("Unable to open input file!\n");
        return -1;
    }
    FILE *outputfile = fopen("results.txt", "w");
    if (outputfile == NULL) {
        printf("Unable to open output file!\n");
        return -1;
    }

	int total_v = 0;
	int total_inv = 0;
    
	/* Complete the code here to use 4 threads */
	pthread_t * p_tids;
	struct thread_args in_params[NUM_THREADS];
	/*struct thread_results out_params[NUM_THREADS];*/
    p_tids = (pthread_t *)malloc(sizeof(pthread_t)*NUM_THREADS);

	pthread_mutex_init(&count_mutex, NULL);

	for (int i = 0; i < NUM_THREADS; i++) {
		in_params[i].inputfd = inputfile;
		in_params[i].outputfd = outputfile;
		in_params[i].validid = 1;
		in_params[i].taxid = (char *)malloc(TAX_ID_LENGTH*sizeof(char));
		memset(in_params[i].taxid, '0', TAX_ID_LENGTH*sizeof(char));
		pthread_create(p_tids + i, NULL, thread_task, in_params + i);
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(p_tids[i], NULL);
		/*pthread_join(p_tids[i], (void **)&(out_params[i]));
		printf("Thread ID %lu joined.\n", p_tids[i]);
		total_v += out_params[i].valid_num;
		total_inv += out_params[i].invalid_num;*/

		free(in_params[i].taxid);
	}

	*valid_tax_id_count = valid_num;
	*invalid_tax_id_count = invalid_num;

    printf( "Number of valid tax IDs: %d\n", *valid_tax_id_count );
    printf( "Number of invalid tax IDs: %d\n", *invalid_tax_id_count );
    free( invalid_tax_id_count );
    free( valid_tax_id_count );
	free( p_tids );
    fclose(inputfile);
    fclose(outputfile);
    return 0;
}

/*
 ***************************************
 * Don't change anything below this line
 ***************************************
 */

char *get_next_tax_id(FILE *inputfile)  {
    char *tax_id = malloc( TAX_ID_LENGTH );
    memset(tax_id, 0,  TAX_ID_LENGTH );
    int read = fscanf(inputfile, "%s\n", tax_id);
    if (read == 0 || read == -1) { /* EOF */
        free(tax_id);
        return NULL;
    }
    return tax_id;
}

void write_output(char *tax_id, bool valid, FILE *outputfile) {
    fprintf(outputfile, "%s: %s\n", tax_id, (valid ? "Valid": "Invalid"));
    free(tax_id);
}

bool check_tax_id( char* tax_id ) {
    /* Simulate network delay */
    usleep(5000);
    /* This is not how tax ID checks work; remote server would say yes or no */
    return tax_id[8] != '0';
}
