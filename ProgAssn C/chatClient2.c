/*
Client 2

References:
	https://therighttutorial.wordpress.com/2014/06/09/multi-client-server-chat-application-using-socket-programming-tcp/
	https://bitbucket.org/vidyakv/network-programming/commits/7f19f9145ab9
	https://github.com/rtroxler/C-Chatroom/blob/master/Part2/client.c
	https://github.com/jhauserw3241/CIS525-Networking/blob/master/ProgAssign2/part2/client.c

SSL REFERENCES:
	https://www.hackanons.com/2018/09/full-duplex-encrypted-chat-server-using.html
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
#include <openssl/ssl.h>
#include <openssl/err.h>

#define C_MAX 101
#define NAME_MAX 25
#define MSG_MAX 800

uint16_t get_port_from_serv_info(const char *serv_info);
int str_to_int(const char *s); 
int serv_fd;
char name[NAME_MAX];


void init_openssl()
{ 
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    return ctx;
}
SSL_CTX* InitCTX(void)     /*creating and setting up ssl context structure*/
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

void ShowCerts(SSL* ssl)  /*show the ceritficates to server and match them but here we are not using any client certificate*/
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

int main(int args, char **argv)
{
	SSL_CTX *ctx;
	int dir_fd, serv_fd;
	struct sockaddr_in dir_addr, serv_addr;
	char msg[MSG_MAX], serv_name[NAME_MAX], serv_info[MSG_MAX];
	int nread; /* number of characters */
	fd_set master_fd_set, working_fd_set;
	uint16_t serv_port;
	pid_t cpid;

	ctx=InitCTX();

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
		
	/*///////////////////////////////////////////////
	* SETUPS FOR SSL CONNECTION TO DIR
	*////////////////////////////////////////////////
	ssl = SSL_new(ctx); //create new SSL connection state
	SSL_set_fd(ssl, dir_fd); //attach socket descriptor
	if(SSL_connect(ssl) == -1) //Performs the connection
		ERR_print_errors_fp(stderr);
	else //else, begin input for dir server to get selected server 
	{
	printf("Connected to DIR with %s encryption\n", SSL_get_cipher(ssl));
	ShowCerts(ssl); /* get any certs */
	/*///////////////////////////////////////////////
	* END SETUP FOR SSL CONNECTION TO DIR
	*////////////////////////////////////////////////

	char test[MSG_MAX];
	memset(test, 0, MSG_MAX);
	snprintf(test, MSG_MAX, "Get directory");

	/* Send the user's name to the server. */
	//write(dir_fd, test, MSG_MAX);
	SSL_write(ssl, test, MSG_MAX); //Replaced with SSL_Write();
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
	}
	
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
	
	/*/////////////////////////////////////////////////////
	* BEGIN SSL CONNECTION SETUP TO SERVER RATHER THAN DIR
	*//////////////////////////////////////////////////////
	
	ssl = SSL_new(ctx); //Create new SSL connection state for server, rather than Dir server
	SSL_set_fd(ssl, serv_fd); // attach new socket descriptor
	if(SSL_connect(ssl) == -1) //perform connection to server.
		ERR_print_errors_fp(stderr);
	else {
	printf("You are now connected to the chat server.\n");
	ShowCerts(ssl); /* get any certs */
	
	/*///////////////////////////////////////////////////
	* END SSL CONNECTION SETUP TO SERVER RATHER THAN DIR
	*////////////////////////////////////////////////////
	
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
		//write(serv_fd, name_msg, 6 + NAME_MAX);
		SSL_write(ssl, name_msg, 6 + NAME_MAX); //Replaced with SSL_Write();
		
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
					fgets(msg, MSG_MAX, stdin);
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
						//write(serv_fd, temp, MSG_MAX);
						SSL_write(ssl, temp, MSG_MAX); //Replaced with SSL_Write();
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
	
	close(serv_fd);

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