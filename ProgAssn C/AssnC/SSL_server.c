/* ----------------------------------------------------------------------------------------------*/
/* server.c 
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

int dir_fd;
SSL* SSL_DIR;

void exit_shell(int signum);

int main(int argc, char *argv[])
{
	SSL_library_init();
	int listen_fd, new_fd, CLI_LEN, childpid, nnames;
	struct sockaddr_in cli_addr, serv_addr, dir_addr;
	char msg[MSG_MAX];
	char request;
	int num, ret_val;
	fd_set master_fd_set, working_fd_set;
	char names[CLI_MAX][NAME_MAX];
	char server_name[NAME_MAX];
	uint16_t port;
	char filename[25];
	char certName[100];
	char keyname[30];
	SSL** sslConnections = calloc(FD_SETSIZE, sizeof(SSL*));
	SSL* SSL_Connect;

	
	signal(SIGINT, exit_shell);

	// Verify that the correct num of args were given
	if(argc != 3) {
		printf("Incorrect number of arguments.\n");
		printf("You need to structure the command as seen below:\n");
		printf("    ./server 'Chat server name' <port num>\n");
		exit(1);
	}
	// Get server name
	if(strlen(argv[1]) <= NAME_MAX) {
		memset(server_name, 0, NAME_MAX);
		snprintf(server_name, NAME_MAX, "%s", argv[1]);
	}
	else {
		printf("The server name must be less than 20 characters\n");
		exit(1);
	}

	if(strstr(server_name, ";")) {
		printf("The server name cannot contain a semicolon\n");
		exit(1);
	}

	if(strstr(server_name, ",")) {
		printf("The server name cannot contain a comma\n");
		exit(1);
	}

	// Get port num
	port = str_to_int(argv[2]);

	/*////////////////////////
	* GET CERTIFICATES
	*/////////////////////////
	snprintf(filename, 25, "%s.pem", server_name);
	snprintf(keyname, 30, "%skey.pem", server_name);
	FILE* fp;
	fp = fopen(filename, "r"); 
	if (fp == NULL){
		printf("something's fucky with the file pointer\n");
		return -1;
	}
	/*/////////////////
	* READ IN THE CERT
	*//////////////////
	X509* cert = PEM_read_X509(fp, NULL, NULL, NULL);
	if (cert == NULL){
		printf("borked the cert\n");
		return -1;
	}

	/*/////////////////////
	* BEGIN SSL CONNECTION
	*/////////////////////	
	const SSL_METHOD* method = SSLv23_method();
	SSL_CTX* sslContext = SSL_CTX_new(SSLv23_method());
	
	/*//////////////////////////////
	* CHECK IF CONNECTION WILL FAIL
	*///////////////////////////////
	if (sslContext == NULL){
		printf("SSL Context Generation Failed\n");
		return -1;
	}
	SSL_DIR = SSL_new(sslContext);
	if (SSL_DIR == NULL){
		printf("SSL Connection Generation Failed\n");
		return -1;
	}
	/* Initialize the list of active sockets */
	FD_ZERO(&master_fd_set);
	FD_ZERO(&working_fd_set);

	/* Create communication endpoint */
	if((dir_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket to directory");
		return(1);
	}
	/* Connect to the directory server */
	memset((char *) &dir_addr, 0, sizeof(dir_addr));
	dir_addr.sin_family 			= AF_INET;
	dir_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	dir_addr.sin_port 				= htons(DIR_TCP_PORT);

	if(connect(dir_fd, (struct sockaddr *) &dir_addr, sizeof(dir_addr)) < 0) {
		perror("server: can't connect to directory");
		return(1);
	}
	
	/*///////////////////////////////
	* SSL CONNECTION AND VERIFICATION
	*////////////////////////////////	
	if (SSL_set_fd(SSL_DIR, dir_fd) != 1){
		printf("SSL and FD Error\n");
		return -1;
	}
	if (SSL_connect(SSL_DIR) != 1){
		printf("SSL Connect Error");
		return -1;
	}
	/*/////////////////////////////
	* CHECK THE READ OF CERTIFICATE
	*//////////////////////////////
	if (X509_NAME_get_text_by_NID(X509_get_subject_name(SSL_get_peer_certificate(SSL_DIR)), 13, certName, 99) == -1){
		printf("certificate name could not be read\n");
		return -1;
	}
	if (strcmp(certName, "directory") != 0){
		printf("certificate doesn't match\n");
		return -1;
	}

	/*////////////////////
	* REG WITH DIR SERVER
	*/////////////////////
	bool regRecvd = false;
	while(regRecvd == false) {
		char regMsg[MSG_MAX];
		memset(regMsg, 0, MSG_MAX);
		snprintf(regMsg, MSG_MAX, "Register server:%s;%d", server_name, port);
		printf("chatserver test\n");

		// Send registration to directory server
		SSL_write(SSL_DIR, regMsg, MSG_MAX);

		// Get confirmation of registration
		memset(msg, 0, MSG_MAX);
		SSL_read(SSL_DIR, msg, MSG_MAX);
		if(strcmp(msg, "registered") == 0) {
			regRecvd = true;
		}
		else {
			printf("%s\n", msg);
			perror("Could not register with directory server\n");
			return(1);
		}
	}

	/* Create communication endpoint */
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		return(1);
	}
	/* Bind socket to local address */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family 			= AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port 				= htons(port);

	if(bind(listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: can't bind local address");
		return(1);
	}
	// Listen for connection from client
	listen(listen_fd, 5);
	FD_SET(listen_fd, &master_fd_set);

	/*//////////////////////////////////////
	* BEGIN FOR LOOP FOR CHAT COMMUNICATION
	*///////////////////////////////////////
	for(;;)
	{
		working_fd_set = master_fd_set;

		// Wait for client to send message
		if(select(FD_SETSIZE, &working_fd_set, NULL, NULL, NULL) < 0) {
			perror("Failure when doing select");
			exit(EXIT_FAILURE);
		}

		// Handle when a socket has something to do
		for(int i = 0; i < FD_SETSIZE; ++i) {
			memset(msg, 0, MSG_MAX);
			if(FD_ISSET(i, &working_fd_set)) {
				// Handle new client wanting to connect
				if(i == listen_fd) {
					/* Accept a new connection request. */
					CLI_LEN = sizeof(cli_addr);
					new_fd = accept(listen_fd, (struct sockaddr *) &cli_addr, &CLI_LEN);
					if(new_fd < 0) {
						perror("server: accept error");
						return(1);
					}
					/*////////////////////////////////
					 * BEGIN SSL CONNECTION TO CLIENT
					 *////////////////////////////////
					sslContext = SSL_CTX_new(SSLv23_method());
					if (sslContext == NULL){
						printf("SSL Context Generation Failed\n");
						return -1;
					}
					if (SSL_CTX_use_certificate(sslContext, cert) != 1){
						printf("SSL and Certificate Error\n");
						return -1;
					}
					if (SSL_CTX_use_PrivateKey_file(sslContext, keyname, SSL_FILETYPE_PEM) != 1){
						printf("SSL and Key Error\n");
						return -1;
					}
					/*////////////////////////////////
					 * SSL CONNECTION ERROR CHECKING
					 *////////////////////////////////
					SSL_Connect = SSL_new(sslContext);
					if (SSL_Connect == NULL){
						printf("SSL Connection Generation Failed\n");
						return -1;
					}

					if (SSL_set_fd(SSL_Connect, new_fd) != 1){
						printf("SSL fd Error\n");
						return -1;
					}
					if (SSL_accept(SSL_Connect) != 1){
						printf("SSL Accept Error\n");
						return -1;
					}
					
					sslConnections[new_fd] = SSL_Connect;
					/* Add the file descriptor to the list of file descriptors */
					FD_SET(new_fd, &master_fd_set);
				}
				// Handle client sending message
				else {
					SSL_read(sslConnections[i], msg, MSG_MAX);
					printf("Client msg: %s\n", msg);

					if(strstr(msg, "Name: "))
					{
						bool nameAlreadyUsed = false;
						char name[NAME_MAX];
						memcpy(name, &msg[6], NAME_MAX);
						for(int v = 0; v < NAME_MAX; v++) {
							if(name[i] == '\n') {
								memset(&name[i], 0, 1);
							}
						}
						// Check if the name is already used
						for(int q = 0; q < nnames; q++) {
							if(strncmp(name, names[q], NAME_MAX) == 0)
							{
								nameAlreadyUsed = true;
							}
						}
						// CHECK IF NAME IS USED ALREADY
						if(nameAlreadyUsed) {
							// Send verification to client
							SSL_write(sslConnections[i], "bad", MSG_MAX);
						}
						else {
							// Send verification to client
							if(nnames == 0) {
								SSL_write(sslConnections[i], "good; You are the first user to join the chat.", MSG_MAX);
							}
							else {
								SSL_write(sslConnections[i], "good", MSG_MAX);
							}

							// Add name to list of names
							strcpy(names[nnames], name);
							nnames++;
							// Tell all current users about new user
							for(int w = 0; w < FD_SETSIZE; ++w) {
								if((w != i) && (w != listen_fd) && (w != dir_fd)) {
									if (sslConnections[w] != NULL){
										char newClientAnnouncement[MSG_MAX];
										memset(newClientAnnouncement, 0, MSG_MAX);
										snprintf(newClientAnnouncement, MSG_MAX, "%s has joined the chat.\n", name);
										SSL_write(sslConnections[w], newClientAnnouncement, MSG_MAX);
									}
								}
							}
						}

						continue;
					}
					/*///////////////////////
					* START SSL SHUTDOWN
					*////////////////////////
					if(strstr(msg, "quit"))
					{
						SSL_shutdown(sslConnections[i]);
						SSL_free(sslConnections[i]);
						sslConnections[i] = NULL;
						close(i);
						// Remove the fd from the master set
						FD_CLR(i, &master_fd_set);
					}
					for(int j = 0; j < FD_SETSIZE; ++j) {
							if (sslConnections[j] != NULL){
								SSL_write(sslConnections[j], msg, MSG_MAX);
							}
					}
				}	
			}
		}
	}
}
void exit_shell(int signum) {
	char temp[MSG_MAX];
	memset(temp, 0, MSG_MAX);
	snprintf(temp, MSG_MAX, "De-register server");
	SSL_write(SSL_DIR, temp, MSG_MAX);
	SSL_shutdown(SSL_DIR);
	SSL_free(SSL_DIR);
	close(dir_fd);
	exit(1);
}
