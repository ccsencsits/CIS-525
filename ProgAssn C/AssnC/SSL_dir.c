/* ----------------------------------------------------------------------------------------------*/
/* dir.c 
/*
/* references: https://www.hackanons.com/2018/09/full-duplex-encrypted-chat-server-using.html
https://wiki.openssl.org/index.php/SSL/TLS_Client#Implementation
/*-----------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "inet.h"
#include "shared.c"
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
SSL_CTX* sslContext;

int main(int argc, char **argv)
{
	SSL_library_init();
	int listen_fd, new_sock_fd, clilen, nnames, bytesRead;
	struct sockaddr_in cli_addr, dir_addr;
	char msg[MSG_MAX];
	fd_set master_fd_set, working_fd_set;
	char names[CLI_MAX][NAME_MAX], serv_info[CLI_MAX][MSG_MAX];
	SSL** sslConnections = calloc(FD_SETSIZE, sizeof(SSL*));

	/*/////////////////////
	* LOAD CERTIFICATE
	*//////////////////////
	FILE* fp;
	fp = fopen("certificate", "r");
	if (fp == NULL){
		printf("something's fucky with the file pointer\n");
		return -1;
	}
	X509* cert = PEM_read_X509(fp, NULL, NULL, NULL);
	if (cert == NULL){
		printf("borked the cert\n");
		return -1;
	}
	/*/////////////////////
	* END: LOAD CERTIFICATE
	*//////////////////////
	
	/*/////////////////////
	* CREATE SSL CONENCTION
	*//////////////////////
	const SSL_METHOD* meth = SSLv23_method();
	sslContext = SSL_CTX_new(meth);
	if (sslContext == NULL){
		printf("SSL Context Generation Failed\n");
		return -1;
	}
	/*///////////////////////////////////////////////
	* CHECK CERT AND SSL CONTEXT TO SEE IF COMPATIBLE
	*////////////////////////////////////////////////
	if (SSL_CTX_use_certificate(sslContext, cert) != 1){
		printf("SSL and Certificate Error\n");
		return -1;
	}
	if (SSL_CTX_use_PrivateKey_file(sslContext, "key", SSL_FILETYPE_PEM) != 1){
		printf("SSL and Key Error\n");
		return -1;
	}
	/*////////////////////////
	* CHECK FOR CONNECION GEN
	*/////////////////////////
	SSL* sslConnection = SSL_new(sslContext);
	if (sslConnection == NULL){
		printf("SSL Connection Generation Failed\n");
		return -1;
	}
	/* Initialize the list of active sockets */
	FD_ZERO(&master_fd_set);
	FD_ZERO(&working_fd_set);

	/* Create communication endpoint */
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		return(1);
	}
	/* Bind socket to local address */
	memset((char *) &dir_addr, 0, sizeof(dir_addr));
	dir_addr.sin_family 			= AF_INET;
	dir_addr.sin_addr.s_addr	= htonl(INADDR_ANY);
	dir_addr.sin_port 				= htons(DIR_TCP_PORT);

	if(bind(listen_fd, (struct sockaddr *) &dir_addr, sizeof(dir_addr)) < 0) {
		perror("directory: can't bind local address");
		return(1);
	}
	// Listen for connection from client or server
	listen(listen_fd, 5);
	FD_SET(listen_fd, &master_fd_set);
	
	/*//////////////////////////////////////////
	* BEGIN FOR LOOP TO LOOP TROUGH TOPIC STUFFS
	*///////////////////////////////////////////
	for(;;)
	{
		working_fd_set = master_fd_set;

		// Wait for communication from fd
		if(select(FD_SETSIZE, &working_fd_set, NULL, NULL, NULL) < 0) {
			perror("Failure when doing select");
			exit(EXIT_FAILURE);
		}

		// Handle when a socket has something to do
		for(int i = 0; i < FD_SETSIZE; ++i) {
			memset(msg, 0, MSG_MAX);
			if(FD_ISSET(i, &working_fd_set) != 0) {
				// Handle new client or server wanting to connect
				if(i == listen_fd) {
					printf("Someone is trying to connect to dir server\n");
					
					/* Accept a new connection request. */
					clilen = sizeof(cli_addr);
					new_sock_fd = accept(listen_fd, (struct sockaddr *) &cli_addr, &clilen);
					if(new_sock_fd < 0) {
						perror("directory: accept error");
						return(1);
					}
					/*/////////////////////////////////////////////
					 * GEN NEW SSL CTX  FOR THE NEEDED TOPIC SERVER
					 *////////////////////////////////////////////
					sslContext = SSL_CTX_new(meth);
					if (sslContext == NULL){
						printf("SSL Context Generation Failed\n");
						return -1;
					}
					/*/////////////////////////////////////////////
					 * CHECK NEW TOPIC SERVERS CERTIFICATE
					 *////////////////////////////////////////////
					if (SSL_CTX_use_certificate(sslContext, cert) != 1){
						printf("SSL and Certificate Error\n");
						return -1;
					}
					/*/////////////////////////////////////////////
					 * CHECK NEW TOPIC SERVERS KEY
					 *////////////////////////////////////////////
					if (SSL_CTX_use_PrivateKey_file(sslContext, "key", SSL_FILETYPE_PEM) != 1){
						printf("SSL and Key Error\n");
						return -1;
					}
					/*/////////////////////////////////////////////
					 * BEGIN NEW SSL CONNECTION
					 *////////////////////////////////////////////
					SSL* sslConnection = SSL_new(sslContext);
					if (sslConnection == NULL){
						printf("SSL Connection Generation Failed\n");
						return -1;
					}

					if (SSL_set_fd(sslConnection, new_sock_fd) != 1){
						printf("SSL fd Error\n");
						return -1;
					}
					if (SSL_accept(sslConnection) != 1){
						printf("SSL Accept Error\n");
						return -1;
					}
					printf("Accept incoming connection\n");

					sslConnections[new_sock_fd] = sslConnection;
					/* Add the file descriptor to the list of file descriptors */
					FD_SET(new_sock_fd, &master_fd_set);					
				}
				// Handle client or server asking to do an action
				else {
					SSL_read(sslConnections[i], msg, MSG_MAX);//REPLACED REVIEVE WITH SSL_READ TO SECURELY READ IN DATA
					printf("%s\n", msg);

					// Get directories
					if(!strcmp(msg, "Get directory"))
					{
						printf("Get directory\n");

						// Get list of active chat servers
						memset(msg, 0, MSG_MAX);
						get_dir_list(msg, serv_info, nnames);

						// Send list to client
						SSL_write(sslConnections[i], msg, strlen(msg));
						//send(i, msg, strlen(msg), 0);

					/*/////////////////////////////////////////////
					 * BEGIN SHUTDOWN OF SSL STUFFS
					 *////////////////////////////////////////////						
					    SSL_shutdown(sslConnections[i]);
						SSL_free(sslConnections[i]);
						sslConnections[i] = NULL;
						close(i);	

						// Remove the fd from the master set
						FD_CLR(i, &master_fd_set);
					}
					// Register a chat server
					else if(strstr(msg, "Register server")) {
						printf("Register chat server\n");
						// Parse out server info
						char info[MSG_MAX];
						memset(info, 0, MSG_MAX);
						memcpy(info, &msg[16], MSG_MAX - 16);

						bool nameAlreadyUsed = false;

						// Parse out name
						char name[NAME_MAX];
						memset(name, 0, NAME_MAX);
						get_name_from_serv_info(name, info);

						// Check if the name is already used
						for(int q = 0; q < nnames; q++) {
							if(strncmp(name, names[q], NAME_MAX) == 0)
							{
								nameAlreadyUsed = true;
							}
						}

						// Handle whether or not the name is already used
						if(nameAlreadyUsed) {
							SSL_write(sslConnections[i], "Chat server name already used", MSG_MAX);
						}
						else {
							SSL_write(sslConnections[i], "registered", MSG_MAX);//SSL TO VERIFY REGISTERED CHAT
							strcpy(names[nnames], name);
							strcpy(serv_info[nnames], info);
							nnames++;
						}
					}
					// DE REG CHAT SERVER
					else if (strstr(msg, "De-register server")) {
						printf("De-register chat server\n");

						if(strstr(msg, "quit"))
						{
							/*/////////////////////////////////////////////
							 * END SSL TO DE REGISTER CHAT
							 *////////////////////////////////////////////
							SSL_shutdown(sslConnections[i]);
							SSL_free(sslConnections[i]);
							sslConnections[i] = NULL;
							close(i);

							// Remove the fd from the master set
							FD_CLR(i, &master_fd_set);
						}
					}
					// Handle error case
					else {
						SSL_write(sslConnections[i], "Invalid request", MSG_MAX);
					}	
				}	
			}
		}
	}
}
