/*----------------------------------------------------------*/
/* client.c                   */
/*
/* references: https://www.hackanons.com/2018/09/full-duplex-encrypted-chat-server-using.html
https://wiki.openssl.org/index.php/SSL/TLS_Client#Implementation
/*----------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include "inet.h"
#include "shared.c"
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#define NAME_MAX 20
#define MSG_MAX 1000
#define FAIL 1

void exit_shell(int signum);

int serv_fd;
char name[NAME_MAX];
SSL* SSL_Connect;

int main(int args, char **argv)
{
	SSL_library_init();
	int dir_fd, serv_fd;
	struct sockaddr_in dir_addr, serv_addr;
	char msg[MSG_MAX], serv_name[NAME_MAX], serv_info[MSG_MAX];
	int nread; /* number of characters */
	fd_set master, wip;
	uint16_t serv_port;
	char certName[110];
	char SERV_NAME[35];

	signal(SIGINT, exit_shell);

	/* Initialize lists of sockets */
	FD_ZERO(&master);
	FD_ZERO(&wip);

	printf("Start setup of chat server\n");

	/* Set up the address of the server to be contacted. */
	memset((char *) &dir_addr, 0, sizeof(dir_addr));
	dir_addr.sin_family				= AF_INET;
	dir_addr.sin_addr.s_addr	= inet_addr(DIR_HOST_ADDR);
	dir_addr.sin_port					= htons(DIR_TCP_PORT);

	/*////////////////////////////////////
	*SETUP SSL CONNECTION ITEMS
	*////////////////////////////////////
	const SSL_METHOD* method = SSLv23_method();
	SSL_CTX* sslContext = SSL_CTX_new(method);
	if (sslContext == NULL){
		printf("SSL Context Generation Failed\n");
		return -1;
	}

	SSL_Connect = SSL_new(sslContext);
	if (SSL_Connect == NULL){
		printf("SSL Connection Failed\n");
		return -1;
	}
	/* Create a socket (an endpoint for communication). */
	if((dir_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	  perror("client: can't open stream socket to directory server");
		return(1);
	}
	/* Connect to the server. */
	if(connect(dir_fd, (struct sockaddr *) &dir_addr, sizeof(dir_addr)) < 0) {
	  perror("client: can't connect to the directory server");
		return(1);
	}
	/*////////////////////////
	* BEGIN SSL CONNECTION VERIFICATION
	*/////////////////////////
	if (SSL_set_fd(SSL_Connect, dir_fd) != FAIL){
		printf("ERROR ON SSL FD\n");
		return -1;
	}
	if (SSL_connect(SSL_Connect) != FAIL){
		printf("ERROR: CONNECTION");
		return -1;
	}
	if (X509_NAME_get_text_by_NID(X509_get_subject_name(SSL_get_peer_certificate(SSL_Connect)), 13, certName, 99) == -1){
		printf("certificate name could not be read\n");
		return -1;
	}
	if (strcmp(certName, "directory") != 0){
		printf("certificate doesn't match\n");
		return -1;
	}
	
	/* Add file descriptors */
	FD_SET(STDIN_FILENO, &master);
	printf("CONNECTION SUCCESSFUL: DIR SERVER\n");
	char test[MSG_MAX];
	memset(test, 0, MSG_MAX);
	snprintf(test, MSG_MAX, "Get directory");

	/* Send the user's name to the server. */
	SSL_write(SSL_Connect, test, MSG_MAX);
	memset(msg, 0, MSG_MAX);
	SSL_read(SSL_Connect, msg, MSG_MAX);
	printf("%s\n", msg);
	memset(serv_info, 0, MSG_MAX);

	// Get server info from user
	printf("Select server: ");
	fgets(serv_info, MSG_MAX, stdin);
	for(int i = 0; i < MSG_MAX; i++) {
		if(serv_info[i] == '\n') {
			memset(&serv_info[i], 0, 1);
		}
	}

	printf("Server name: %s\n", serv_info);
	// Get server port
	serv_port = get_port_from_serv_info(serv_info);
	get_name_from_serv_info(SERV_NAME, serv_info);

	printf("Port #: %d\n", serv_port);

	/*///////////////////////////////////
	* BEGIN SSL SHUTDOWN SEQUENCE FOR DIR
	*////////////////////////////////////
	SSL_shutdown(SSL_Connect);
	SSL_free(SSL_Connect);
	close(dir_fd);


	/*/////////////////////////////////
	* BEGIN CONNECTION TO TOPIC SERVER
	*//////////////////////////////////
	/* Set up the address of the server to be contacted. */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family				= AF_INET;
	serv_addr.sin_addr.s_addr	= inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port					= htons(serv_port);

	/*////////////////////////
	* BEGIN CONNECTION ITEMS
	*/////////////////////////
	sslContext = SSL_CTX_new(SSLv23_method());
	if (sslContext == NULL){
		printf("SSL gen failed\n");
		return -1;
	}
	SSL_Connect = SSL_new(sslContext);
	if (SSL_Connect == NULL){
		printf("SSL Connection Failed\n");
		return -1;
	}

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
	/*//////////////////////////////////////////////////////////////////////
	 * BEGIN: SSL ERROR CHECKING FOR CONNECTION, CERTS, AND FD for the main server
	 *//////////////////////////////////////////////////////////////////////
	if (SSL_set_fd(SSL_Connect, dir_fd) != FAIL){
		printf("SSL and FD Error\n");
		return -1;
	}
	//check connection if error
	if (SSL_connect(SSL_Connect) != FAIL){
		printf("SSL Connect Error");
		return -1;
	}
	//check cert to see if was read properly
	if (X509_NAME_get_text_by_NID(X509_get_subject_name(SSL_get_peer_certificate(SSL_Connect)), 13, certName, 99) == -1){
		printf("certificate name could not be read\n");
		return -1;
	}
	//compare if it is the right certificate
	if (strcmp(certName, SERV_NAME) != 0){
		printf("certificate doesn't match\n");
		return -1;
	}
	/*//////////////////////////////////////////////////////////////////////
	 * END: SSL ERROR CHECKING FOR CONNECTION, CERTS, AND FD for the main server
	 *//////////////////////////////////////////////////////////////////////
	
	/*////////////////////////
	* END CONNECTION ITEMS
	*/////////////////////////	

	/* Add file descriptors */
	FD_SET(serv_fd, &master);

	printf("CONNECTION SUCCESSFUL: TOPIC SERVER.\n");

	bool nameApproved = false;
	while(nameApproved != true) {
	printf("Your name: ");
	  fgets(name, NAME_MAX, stdin);
		for(int i = 0; i < NAME_MAX; i++) {
			if(name[i] == '\n') {
				memset(&name[i], 0, 1);
			}
		}
		// Disallow a name that would confuse the server
		if(name[0] == 'N' && name[1] == 'a' && name[2] == 'm' && name[3] == 'e') {
			printf("That name is not allowed. Please choose a different name.\n");
			continue;
		}
		
		// Prep the message to be sent to the server
		char name_msg[6 + NAME_MAX];
		memset(name_msg, 0, 6 + NAME_MAX);
		snprintf(name_msg, 6 + NAME_MAX, "Name: %s\n", name);

		/* Send the user's name to the server. */
		SSL_write(SSL_Connect, name_msg, 6 + NAME_MAX);

		memset(msg, 0, MSG_MAX);
		SSL_read(SSL_Connect, msg, MSG_MAX);

		if(strstr(msg, "good") != NULL) {
			nameApproved = true;

			if(strstr(msg, "good; ")) {
				char firstPersonMsg[MSG_MAX];
				memcpy(firstPersonMsg, &msg[6], MSG_MAX);
				//printf("%s\n", firstPersonMsg);
			}
		}
		else {
			printf("That name is already taken. Please choose a different name.\n");
		}
	}

	/* Allow the user to chat with other users. */
	for(;;) {
		wip = master;
		
		// Wait for activity on file descriptor
		if(select(FD_SETSIZE, &wip, NULL, NULL, NULL) < 0) {
			perror("select: failure");
			exit(EXIT_FAILURE);
		}

		// Handle when a file descriptor has something to do
		for(int i = 0; i < FD_SETSIZE; ++i) {
			if(FD_ISSET(i, &wip)) {
				// Hanlde input from user
				if(i == STDIN_FILENO) {
					// Get the message from the user
					memset(msg, 0, MSG_MAX);
					fgets(msg, MSG_MAX, stdin);
					msg[strcspn(msg, "\n")] = 0;

					// Don't allow message that cuts off connection to server
					if(msg[0] == 'q' && msg[1] == 'u' && msg[2] == 'i' && msg[3] == 't')
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
						SSL_write(SSL_Connect, temp, MSG_MAX);
					}
				}
				// Handle message from server
				else {
						SSL_read(SSL_Connect, msg, MSG_MAX);
						printf("%s", msg);
					}
				}
			}
		}

	}
	
	/*///////////////////////////////////
	* BEGIN SSL SHUTDOWN SEQUENCE FOR TOPIC
	*////////////////////////////////////
	SSL_shutdown(SSL_Connect);
	SSL_free(SSL_Connect);
	close(serv_fd);

	return(0);	/* Exit if response is 4. */
}

void exit_shell(int signum) {
	char temp[NAME_MAX + 6];
	memset(temp, 0, NAME_MAX + 6);
	snprintf(temp, NAME_MAX + 6, "%s: quit", name);
	SSL_write(SSL_Connect, temp, MSG_MAX);
	SSL_shutdown(SSL_Connect);
	SSL_free(SSL_Connect);
	close(serv_fd);
	exit(1);
}
