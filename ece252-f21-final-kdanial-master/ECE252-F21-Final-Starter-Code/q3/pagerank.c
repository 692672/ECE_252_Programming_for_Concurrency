#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include "q3helper.h"

void pagerank(link_t l[], int nLinks, double rank[], int nPages, double delta,
		int iterations, int nProcesses) {
	// compute number of outgoing links for each page
	int outDegree[nPages];
	for(int i=0; i<nPages; i++) outDegree[i] = 0;
	for(int j=0; j<nLinks; j++) outDegree[l[j].src]++;

	double * r; // ranks
	double * s; // new ranks

	// my variables
	int status = 0;
	pid_t pid = 0;
	pid_t processes[nProcesses];

	barrier_t * barr;
	
	int shmid1 = shmget(IPC_PRIVATE, sizeof(double) * nPages, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int shmid2 = shmget(IPC_PRIVATE, sizeof(double) * nPages, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int shmid3 = shmget(IPC_PRIVATE, sizeof(barrier_t), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	
	if(shmid1 == -1 || shmid2 == -1) {
		printf("ERROR - cannot allocate shared memory\n");
	}

	r = (double *)shmat(shmid1, NULL, 0);
	s = (double *)shmat(shmid2, NULL, 0);
	barr = (barrier_t *)shmat(shmid3, NULL, 0);

	bInit(barr, nProcesses);

	// initial guess
	for(int i=0; i<nPages; i++) r[i] = 1.0/nPages;

	// FORK PROCESSES HERE
	for(int t = 0; t < nProcesses; t++) {
		pid = fork();
		
		if (pid == 0) {	
			// Power method
			for(int k=0; k<iterations; k++) {
          		// calculate new values, placing them in s[]
				
				int pages_start = t * (nPages / nProcesses);
				int pages_end = pages_start + (nPages / nProcesses);

				if (t == nProcesses - 1)
					pages_end = nPages;

				for(int i=pages_start; i<pages_end; i++) {
              		s[i] = (1.0-delta)/nPages;
              		for(int j=0; j<nLinks; j++)
                  		if(l[j].dst == i)
                      		s[i] += delta * r[l[j].src] / outDegree[l[j].src];
          		}
        		// copy s[] to r[] for next iteration
        		for(int i=0; i<nPages; i++)
            		r[i] = s[i];
				
				bSync(barr);
			}
			exit(0);
		} else if (pid > 0) {
			processes[t] = pid;
		} else {
			abort();
		}
	}

	// JOIN PROCESSES HERE
	for(int i = 0; i < nProcesses; i++)
		waitpid(processes[i], &status, 0);

	for(int i=0; i<nPages; i++)
		rank[i] = r[i];
}

/*double * power_method(int iterations, int nPages, int nLinks, double delta, 
		int outDegree, link_t l[]) {

	double r[nPages]; // ranks
    double s[nPages]; // new ranks
	
	// power method
    for(int k=0; k<iterations; k++) {
        // calculate new values, placing them in s[]
        for(int i=0; i<nPages; i++) {
            s[i] = (1.0-delta)/nPages;
            for(int j=0; j<nLinks; j++)
              if(l[j].dst == i)
                    s[i] += delta * r[l[j].src] / outDegree[l[j].src];
        }

        // copy s[] to r[] for next iteration
        for(int i=0; i<nPages; i++)
            r[i] = s[i];
    }

	return r;
} */

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("usage: %s <n_process> <link_file>\n", argv[0]);
		return 0;
	}

	// read in links file
	link_t *links;
	char (*names)[NAME_MAX];
	size_t n_links, n_pages;
	if(!readLinks(argv[2], &links, &n_links, &names, &n_pages)) {
		printf("failed to read links file %s\n", argv[2]);
		return 0;
	}

	// start timer
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double startTime = (tv.tv_sec) + tv.tv_usec/1000000.;

	// run pagerank
	double *rank = malloc(n_pages * sizeof(double));
	double delta = 0.80; // convergence rate
	double iterations = 1000;
	int processes = atoi(argv[1]);
	if(processes < 1 || processes > 4) {
		printf("processes %d not in [1,4]\n", processes);
		return 0;
	}
	pagerank(links, n_links, rank, n_pages, delta, iterations, processes);

	// stop timer
	gettimeofday(&tv, NULL);
	double endTime = (tv.tv_sec) + tv.tv_usec/1000000.;
	printf("%s execution time: %.2f seconds\n", argv[0], endTime - startTime);

	// write ranks.txt
	FILE *fout = fopen("ranks.txt", "w");
	for(int i=0; i<n_pages; i++)
		fprintf(fout, "%.8f %s\n", rank[i], names[i]);
	fclose(fout);

	// clean up
	free(links);
	free(names);
	free(rank);
}

