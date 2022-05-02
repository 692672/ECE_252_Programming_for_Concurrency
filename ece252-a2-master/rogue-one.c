#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT "2520"
#define PLANS_FILE "deathstarplans.dat" 

typedef struct {
    char * data;
    int length;
} buffer;

extern int errno;

int sendall(int socket, buffer * buf);

buffer load_plans( );

int main( int argc, char** argv ) {

    if ( argc != 2 ) {
        printf( "Usage: %s IP-Address\n", argv[0] );
        return -1;
    }
    printf("Planning to connect to %s.\n", argv[1]);
	
	buffer buf = load_plans();

    struct addrinfo hints;
    struct addrinfo * res;
	void * received_buffer;
	int status = -1;
	int received;
	int result;
	int sockfd;
	int sent;

	received_buffer = malloc(64);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo(argv[1], PORT, &hints, &res);
	
	if (result == 0) {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		status = connect(sockfd, res->ai_addr, res->ai_addrlen);
		sent = sendall(sockfd, &buf);
		if (sent == 0) {
			received = recv(sockfd, received_buffer, buf.length, 0);
			printf("%s", received_buffer);
		} else {
			printf("TRANSMISSION FAILED");
		}
	}
	close(sockfd);
	free(received_buffer);
	free(buf.data);
    return 0;
}

buffer load_plans( ) {
    struct stat st;
    stat( PLANS_FILE, &st );
    ssize_t filesize = st.st_size;
    char* plansdata = malloc( filesize );
    int fd = open( PLANS_FILE, O_RDONLY );
    memset( plansdata, 0, filesize );
    read( fd, plansdata, filesize );
    close( fd );

    buffer buf;
    buf.data = plansdata;
    buf.length = filesize;

    return buf;
}

int sendall(int socket, buffer * buf) {
	int total = 0;
	int byteleft = buf->length;
	int n;

	while (total < buf->length) {
		n = send(socket, buf->data + total, byteleft, 0);
		if (n == -1) {
			break;
		}
		total += n;
		byteleft -= n;
	}
	buf->length = total;
	return n == -1 ? -1 : 0;
}
