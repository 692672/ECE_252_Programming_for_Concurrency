#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <errno.h>
#include <stdbool.h>

#define PORT_1 "2527"
#define PORT_2 "2528"
#define PORT_3 "2529"
#define SERVER_IP "34.226.202.123"

char* ok_msg = "OK";

void listen_for_connections( int, int, int );

int main( int argc, char** argv ) {
	struct addrinfo hints1;
	struct addrinfo hints2;
	struct addrinfo hints3;
	struct addrinfo * res1;
	struct addrinfo * res2;
	struct addrinfo * res3;
	int service1_sock;
	int service2_sock;
	int service3_sock;
	int status1;
	int status2;
	int status3;

	memset(&hints1, 0, sizeof(hints1));
	memset(&hints2, 0, sizeof(hints2));
	memset(&hints3, 0, sizeof(hints3));

	hints1.ai_family = AF_INET;
	hints1.ai_socktype = SOCK_STREAM;
	hints2.ai_family = AF_INET;
    hints2.ai_socktype = SOCK_STREAM;
	hints3.ai_family = AF_INET;
    hints3.ai_socktype = SOCK_STREAM;

	getaddrinfo(SERVER_IP, PORT_1, &hints1, &res1);
	getaddrinfo(SERVER_IP, PORT_2, &hints2, &res2);
	getaddrinfo(SERVER_IP, PORT_3, &hints3, &res3);

	service1_sock = socket(res1->ai_family, res1->ai_socktype, res1->ai_protocol);
	service2_sock = socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol);
	service3_sock = socket(res3->ai_family, res3->ai_socktype, res3->ai_protocol);

	status1 = connect(service1_sock, res1->ai_addr, res1->ai_addrlen);
	status2 = connect(service2_sock, res2->ai_addr, res2->ai_addrlen);
	status3 = connect(service3_sock, res3->ai_addr, res3->ai_addrlen);

	listen_for_connections(service1_sock, service2_sock, service3_sock);

	close(service1_sock);
	close(service2_sock);
	close(service3_sock);
    printf("Done!\n");
    return 0;
}

/* This is the code from lecture 26 as your starter code; modify as needed */
void listen_for_connections( int service1_sock, int service2_sock, int service3_sock ) {
    int nfds = 1 + (service1_sock > service2_sock
            ? service1_sock > service3_sock ? service1_sock : service3_sock
            : service2_sock > service3_sock ? service2_sock : service3_sock);

    fd_set s;
    struct timeval tv;
    bool quit = false;
	int send_status;
	int num_messages = 0;
	int len_buf;
	int bytes = 0;
	char * buffer;
    
	while( !quit ) {

        FD_ZERO( &s );
        FD_SET( service1_sock, &s );
        FD_SET( service2_sock, &s );
        FD_SET( service3_sock, &s );

        tv.tv_sec = 30;
        tv.tv_usec = 0;

        int res = select( nfds, &s, NULL, NULL, &tv );
        if ( res == -1 ) { /* An error occurred */
            printf( "An error occurred in select(): %s.\n", strerror( errno ) );
            quit = 1;
        } else if ( res == 0 ) { /* 0 sockets had events occur */
			printf( "Still waiting; nothing occurred recently.\n" );
        } else { /* Things happened */
            if ( FD_ISSET( service1_sock, &s ) ) {
				bytes = recv(service1_sock, &len_buf, sizeof(int), 0);

				if (bytes > 0) {
					send_status = send(service1_sock, ok_msg, strlen(ok_msg), 0);
					len_buf = ntohl(len_buf);

					buffer = (char *)malloc(sizeof(char)*len_buf + 1);
					recv(service1_sock, buffer, len_buf, 0);
					buffer[sizeof(char)*len_buf] = '\0';
					num_messages += 1;
					printf("%s\n", buffer);
					free(buffer);
					if (num_messages == 3)
                    	quit = true;
				}
            }
            if ( FD_ISSET( service2_sock, &s ) ) {
				bytes = recv(service2_sock, &len_buf, sizeof(int), 0);
				
				if (bytes > 0) {
					send_status = send(service2_sock, ok_msg, strlen(ok_msg), 0);
					len_buf = ntohl(len_buf);

					buffer = (char *)malloc(sizeof(char)*len_buf + 1);
                	recv(service2_sock, buffer, len_buf, 0);
					buffer[sizeof(char)*len_buf] = '\0';
					num_messages += 1;
					printf("%s\n", buffer);
					free(buffer);
					if (num_messages == 3)
                    	quit = true;
				}
            }
            if ( FD_ISSET( service3_sock, &s ) ) {
				bytes = recv(service3_sock, &len_buf, sizeof(int), 0);
				
				if (bytes > 0) {
					send_status = send(service3_sock, ok_msg, strlen(ok_msg), 0);
					len_buf = ntohl(len_buf);

				
					buffer = (char *)malloc(sizeof(char)*len_buf + 1);
					recv(service3_sock, buffer, len_buf, 0);
					buffer[sizeof(char)*len_buf] = '\0';
					num_messages += 1;
					printf("%s\n", buffer);
					free(buffer);
					if (num_messages == 3)
						quit = true;
				}
            }
        }
    }
}
