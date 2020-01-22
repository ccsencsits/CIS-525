/*
Client 2

References:
	https://therighttutorial.wordpress.com/2014/06/09/multi-client-server-chat-application-using-socket-programming-tcp/
	https://bitbucket.org/vidyakv/network-programming/commits/7f19f9145ab9
	https://github.com/rtroxler/C-Chatroom/blob/master/Part2/client.c
	https://github.com/jhauserw3241/CIS525-Networking/blob/master/ProgAssign2/part2/client.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include "inetServer.h"
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define C_MAX 101
#define NAME_MAX 25
#define MSG_MAX 800

SSL_CTX * Init_CTX();

uint16_t get_port_from_serv_info(const char *serv_info);
int str_to_int(const char *s); 
int serv_fd;
char name[NAME_MAX];

int main(int args, char **argv)
{
	int dir_fd, serv_fd;
	struct sockaddr_in dir_addr, serv_addr;
	char msg[MSG_MAX], serv_name[NAME_MAX], serv_info[MSG_MAX];
	int nread; /* number of characters */
	fd_set master_fd_set, working_fd_set;
	uint16_t serv_port;
	
	SSL_CTX *ctx;
	SSL *ssl;
	int bytes;

	/*Begin SSL Items*/
	SSL_library_init();
	ctx = Init_CTX();
	
	/* Initialize lists of sockets */
	FD_ZERO(&master_fd_set);
	FD_ZERO(&working_fd_set);

	/* Set up the address of the server to be contacted. */
	memset((char *) &dir_addr, 0, sizeof(dir_addr));
	dir_addr.sin_family				= AF_INET;
	dir_addr.sin_addr.s_addr	= inet_addr(DIR_HOST_ADDR);
	dir_addr.sin_port					= htons(DIR_TCP_PORT);

	/* Create a socket (an endpoint for communication). */
	if((dir_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	  perror("client: can't open stream socket to directory server");
		return(1);
	}
	
	/*Contine SSL Items*/
	ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, dir_fd);    /* attach the socket descriptor */
	if ( SSL_connect(ssl) == -1 )   /* perform the connection */
        ERR_print_errors_fp(stderr);
	
	/* Connect to the server. */
	/*if(connect(dir_fd, (struct sockaddr *) &dir_addr, sizeof(dir_addr)) < 0) {
	  perror("client: can't connect to the directory server");
		return(1);
	}*/


	/* Add file descriptors */
	FD_SET(STDIN_FILENO, &master_fd_set);

	//printf("You are now connected to the directory server.\n");
	printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
    ShowCerts(ssl);
	
	char test[MSG_MAX];
	memset(test, 0, MSG_MAX);
	snprintf(test, MSG_MAX, "Get directory");

	/* Send the user's name to the server. */
	//write(dir_fd, test, MSG_MAX);
	SSL_write(ssl, test, MSG_MAX);

	printf("Send message to chat dir server\n");

	memset(msg, 0, MSG_MAX);
	nread = read(dir_fd, msg, MSG_MAX);
	if(nread <= 0) {
		printf("Nothing read, nread = %d.\n", nread);
	}
	printf("%s\n", msg);

	memset(serv_info, 0, MSG_MAX);

	// Get info from user
	printf("Select server: ");
	fgets(serv_info, MSG_MAX, stdin);
	for(int i = 0; i < MSG_MAX; i++) {
		if(serv_info[i] == '\n') {
			memset(&serv_info[i], 0, 1);
		}
	}
	close(dir_fd);

	/* Set up the address of the server to be contacted. */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family				= AF_INET;
	serv_addr.sin_addr.s_addr	= inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port					= htons(serv_port);

	/* Create a socket (an endpoint for communication). */
	if((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	  perror("client: can't open stream socket to chat server");
		return(1);
	}

	/* Connect to the server. */
	if(connect(serv_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	  perror("client: can't connect to the chat server");
		return(1);
	}

	/* Add file descriptors */
	FD_SET(serv_fd, &master_fd_set);

	printf("You are now connected to the chat server.\n");

	bool nameApproved = false;
	while(nameApproved != true) {
		// Get name from user
		printf("Your name: ");
	  fgets(name, NAME_MAX, stdin);
		for(int i = 0; i < NAME_MAX; i++) {
			if(name[i] == '\n') {
				memset(&name[i], 0, 1);
			}
		}

		// Disallow a name that would confuse the server
		if(strcmp(name, "Name") == 0) {
			printf("That name is not allowed. Please choose a different name.\n");
			continue;
		}

		// Prep the message to be sent to the server
		char name_msg[6 + NAME_MAX];
		memset(name_msg, 0, 6 + NAME_MAX);
		snprintf(name_msg, 6 + NAME_MAX, "Name: %s\n", name);

		/* Send the user's name to the server. */
		write(serv_fd, name_msg, 6 + NAME_MAX);

		memset(msg, 0, MSG_MAX);
		nread = read(serv_fd, msg, MSG_MAX);
		if(nread <= 0) {
			printf("Nothing read, nread = %d.\n", nread);
		}

		if(strstr(msg, "good")) {
			nameApproved = true;

			if(strstr(msg, "good; ")) {
				char firstPersonMsg[MSG_MAX];
				memcpy(firstPersonMsg, &msg[6], MSG_MAX);
				printf("%s\n", firstPersonMsg);
			}
		}
		else {
			printf("That name is already taken. Please choose a different name.\n");
		}
	}

	/* Allow the user to chat with other users. */
	for(;;) {
		working_fd_set = master_fd_set;

		// Wait for activity on file descriptor
		if(select(FD_SETSIZE, &working_fd_set, NULL, NULL, NULL) < 0) {
			perror("select: failure");
			exit(EXIT_FAILURE);
		}

		// Handle when a file descriptor has something to do
		for(int i = 0; i < FD_SETSIZE; ++i) {
			if(FD_ISSET(i, &working_fd_set)) {
				// Hanlde input from user
				if(i == STDIN_FILENO) {
					// Get the message from the user
					memset(msg, 0, MSG_MAX);
					//fgets(msg, MSG_MAX, stdin);
					SSL_read(ssl, msg, MSG_MAX);
					msg[strcspn(msg, "\n")] = 0;

					// Don't allow message that cuts off connection to server
					if(strcmp(msg, "quit") == 0)
					{
						printf("That message is not allowed\n");
					}
					// Send message to server
					else {
						// Prep the message to be sent to the server
						char temp[NAME_MAX + 2 + MSG_MAX];
						memset(temp, 0, NAME_MAX + 2 + MSG_MAX);
						snprintf(temp, NAME_MAX + 2 + MSG_MAX, "%s: %s\n", name, msg);

						/* Send the user's response to the server. */
						write(serv_fd, temp, MSG_MAX);
					}
				}
				// Handle message from server
				else {
					nread = read(serv_fd, msg, MSG_MAX);
					if(nread > 0) {
						printf("%s", msg);
					} else {
						printf("Nothing read, nread = %d.\n", nread);
					}			
				}
			}
		}

	}
	close(serv_fd);         /* close socket */
    SSL_CTX_free(ctx);        /* release context */
	return(0);	/* Exit if response is 4. */
}
uint16_t get_port_from_serv_info(const char *serv_info) {
	char port[MSG_MAX];
	memset(port, 0, MSG_MAX);

	int index = -1;
	const char *ptr = strchr(serv_info, ';');
	if(ptr) {
		index = ptr - serv_info;
	}

	memcpy(port, &serv_info[index + 1], MSG_MAX);

	return str_to_int(port);
}
int str_to_int(const char *s)
{
	int res = 0;
	while (*s) {
		res *= 10;
		res += *s++ - '0';
	}
	return res;
}

SSL_CTX* Init_CTX()
{   SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}