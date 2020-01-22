/*
Directory Server 2

References:
	https://therighttutorial.wordpress.com/2014/06/09/multi-client-server-chat-application-using-socket-programming-tcp/
	https://bitbucket.org/vidyakv/network-programming/commits/7f19f9145ab9
	https://github.com/jhauserw3241/CIS525-Networking/blob/master/ProgAssign2/part2/dir.c
	https://github.com/rtroxler/C-Chatroom/blob/master/Part2/directory.c
*/
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
#include "inetServer.h"
#include <pthread.h> 

#define C_MAX 101
#define NAME_MAX 25
#define MSG_MAX 800
#define NUM_THREADS 9

void name_retrieval(char *name, const char *serv_info);
void retrieve_dir(char *list, char info[C_MAX][MSG_MAX], const int info_size);
void append_str(char *orig, const char *a); 

//Supplementary Functions to reduce code size and increase efficiency
void name_retrieval(char *name, const char *serv_info) 
{
	int index = -1;
	const char *ptr = strchr(serv_info, ';');
	if(ptr) {
		index = ptr - serv_info;
	}

	snprintf(name, index + 1, "%s", serv_info);
}
void retrieve_dir(char *list, char info[C_MAX][MSG_MAX], const int info_size) 
{
	snprintf(list, 21, "Chat Server Directory");

	for(int i = 0; i < info_size; i++) {
		append_str(list, info[i]);
	}
}
void append_str(char *orig, const char *a) 
{
	char temp[MSG_MAX];
	memset(temp, 0, MSG_MAX);
	snprintf(temp, MSG_MAX, "%s\n%s", orig, a);

	memset(orig, 0, MSG_MAX);
	strcpy(orig, temp);
}

int main(int argc, char **argv)
{
	struct sockaddr_in c_addr, d_addr;
	int listen_fd, new_sfd, c_length, name_nums;
	char names[C_MAX][NAME_MAX], serv_info[C_MAX][MSG_MAX];
	char msg[MSG_MAX];
	fd_set master, wip;
	
	int rc, i;
	pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;
	
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//Initialize the sockets
	FD_ZERO(&master);
	FD_ZERO(&wip);
	
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);//Create endpoint for comms
	//Bind socket to local addr
	d_addr.sin_family = AF_INET;
	d_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	d_addr.sin_port = htons(DIR_TCP_PORT);

	bind(listen_fd, (struct sockaddr *) &d_addr, sizeof(d_addr));

	// Listen for connections
	listen(listen_fd, 5);
	FD_SET(listen_fd, &master);

	for(;;)
	{
		wip = master;

		// Wait for communication from fd
		/*if(select(FD_SETSIZE, &wip, NULL, NULL, NULL) < 0) {
			perror("Failure when doing select");
			exit(EXIT_FAILURE);
		}*/
		FD_ISSET(FD_SETSIZE, &wip);

		// Handle when a socket has something to do
		for(int i = 0; i < FD_SETSIZE; ++i) {
			//memset(msg, 0, MSG_MAX);
			if(FD_ISSET(i, &wip)) {
				// Handle new client or server wanting to connect
				if(i == listen_fd) {
					c_length = sizeof(c_addr);
					pthread_create(&threads[i], &attr, NULL, (void *)i);
					new_sfd = accept(listen_fd, (struct sockaddr *) &c_addr, &c_length);
					if(new_sfd < 0) {
						perror("ERROR: Cant accept new connection");
						return(1);
					}
					printf("Incoming connection: ACCEPTED\n");

					/* Add the file descriptor to the list of file descriptors */
					FD_SET(new_sfd, &master);
				}
				// Handle client or server asking to do an action
				else {
					recv(i, msg, MSG_MAX, 0);

					// Get directories
					if(!strcmp(msg, "Get directory"))
					{
						printf("Getting directory\n");

						// Get list of active chat servers
						memset(msg, 0, MSG_MAX);
						retrieve_dir(msg, serv_info, name_nums);

						send(i, msg, strlen(msg), 0);
						close(i);

						// Remove the fd from the master set
						FD_CLR(i, &master);
					}
					// Register a chat server
					else if(strstr(msg, "Register server")) {
						printf("Registering chat server\n");
						// Parse out server info
						char info[MSG_MAX];
						memset(info, 0, MSG_MAX);
						memcpy(info, &msg[16], MSG_MAX - 16);

						bool name_in_use = false;

						// Parse out name
						char name[NAME_MAX];
						memset(name, 0, NAME_MAX);
						name_retrieval(name, info);

						// Check if the name is already used
						for(int m = 0; m < name_nums; m++) {
							if(strncmp(name, names[m], NAME_MAX) == 0)
							{
								name_in_use = true;
							}
						}

						// Handle whether or not the name is already used
						if(name_in_use) {
							// Send verification to client
							send(i, "Chat server name already used", MSG_MAX, 0);
						}
						else {
							// Send verification to client
							send(i, "Registered", MSG_MAX, 0);

							// Add name to list of names
							strcpy(names[name_nums], name);
							strcpy(serv_info[name_nums], info);
							name_nums++;
						}
					}
					// De-register a chat server
					else if (strstr(msg, "De-register server")) {
						printf("De-register chat server\n");

						// Remove server
						if(strstr(msg, "quit"))
						{
							close(i);

							// Remove the fd from the master set
							FD_CLR(i, &master);
						}
					}
					// Handle error case
					else {
						send(i, "Invalid request", MSG_MAX, 0);
					}	
				}	
			}
		}
	}
}