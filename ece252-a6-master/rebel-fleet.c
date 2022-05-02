#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>
#define PORT 4568

volatile int quit = 0;
char* plansfile = "deathstarplans.dat";
char* plansbuf;
ssize_t filesize;

char* msg_ok = "Success - Death Star Plans Received!\n";
char* err_inc = "Error - Incomplete Data Received.\n";
char* err_long = "Error - More Data Than Expected Received.\n";
char* err_corr = "Error - Data corrupted.\n";

void handle_quit(int sig) {
	quit = 1;
}

int main( int argc, char** argv ) {
    struct stat st;
    stat( plansfile, &st );
    filesize = st.st_size;
    plansbuf = malloc( filesize );
    memset( plansbuf, 0, filesize );
    int fd = open( plansfile , O_RDONLY );
    read( fd, plansbuf, filesize );
    int enable = 1;

	signal(SIGINT, handle_quit);

    /* Create Socket, IPv4 / Stream */
    int sockfd = socket( AF_INET, SOCK_STREAM, 0 ); 

    /* Create the sockaddr in on port */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( PORT ); 
    addr.sin_addr.s_addr = htonl( INADDR_ANY );
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	listen(sockfd, 10);

    struct pollfd pollfds[500];
	pollfds[0].fd = sockfd;
	pollfds[0].events = POLLIN;

	int nfds = 1;
	int timeout = 5 * 1000;

    char * recv_buf[500];
	int received_sizes[500];
	int visited[500];
	for (int i = 0; i < 500; i++) {
		recv_buf[i] = (char *)malloc(1050000);
	}
	memset(visited, 0, sizeof(visited));
	memset(received_sizes, 0, sizeof(received_sizes));

    while(quit != 1) {
	    int res = poll( pollfds, nfds, timeout );
        int clean = 0;
		/*int availablefds = nfds;*/

		if (res == -1) {
			quit = 1;
		} else if (res == 0) {
		    printf("Still listening...\n");
		} else {
		    
			if(pollfds[0].revents & POLLIN) {
				int acceptedsock = 0;
				acceptedsock = accept(sockfd, NULL, NULL);

				if (acceptedsock >= 0) {
					pollfds[nfds].fd = acceptedsock;
					pollfds[nfds].events = POLLIN;
					nfds++;
					fcntl(acceptedsock, F_SETFL, O_NONBLOCK);
				}
			}
        }
		for (int i = 1; i < nfds; i++) {
            if (visited[i]) {
                continue;
            }
            int bytes_received = recv(pollfds[i].fd, recv_buf[i] + received_sizes[i], 1050000 - received_sizes[i], 0);
            if (bytes_received <= 0) {
                if (received_sizes[i] > filesize) {
                    send(pollfds[i].fd, err_long, strlen(err_inc) + 1, 0);
                } else if (strcmp(recv_buf[i], plansbuf) != 0) {
                    send(pollfds[i].fd, err_corr, strlen(err_corr) + 1, 0);
                } else if (received_sizes[i] < filesize) {
                    send(pollfds[i].fd, err_inc, strlen(err_inc) + 1, 0);
                } else{
                    send(pollfds[i].fd, msg_ok, strlen(msg_ok) + 1, 0);
                }

                pollfds[i].fd = -1;
				visited[i] = 1;
            }
            received_sizes[i] += bytes_received;
        }
    }
	
	for (int i = 0; i < 500; i++) {
		free(recv_buf[i]);
	}
    free( plansbuf );
    close( fd );
    return 0;
}

