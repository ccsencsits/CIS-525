/*
Client1

References:
	https://therighttutorial.wordpress.com/2014/06/09/multi-client-server-chat-application-using-socket-programming-tcp/
	https://bitbucket.org/vidyakv/network-programming/commits/7f19f9145ab9
	https://github.com/rtroxler/C-Chatroom/blob/master/Part1/server.c
	https://github.com/jhauserw3241/CIS525-Networking/blob/master/ProgAssign2/part1/server.c
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
#include "inet.h"

#define MSG_MAX 1000
#define NAME_MAX 20
#define PIPE_NUM 2
#define CLI_MAX 100

int main(int argc, char **argv)
{
	int sfd, new_sfd, c_len, num_names;
	struct sockaddr_in cli_addr, serv_addr;
	char msg[MSG_MAX];
	char names[CLI_MAX][NAME_MAX];
	fd_set master, wip;

	/* Initialize the list of active sockets */
	FD_ZERO(&master);
	FD_ZERO(&wip);

	// Create communication endpoint
	sfd = socket(AF_INET, SOCK_STREAM, 0);

	/* Bind socket to local address */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	/*Refernced wordpress for this*/
	serv_addr.sin_family 			= AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port 				= htons(SERV_TCP_PORT);

	bind (sfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	
	// Listen for connection from client
	listen(sfd, 5);
	FD_SET(sfd, &master);

	for(;;)
	{
		wip = master;

		// Wait for client to send message
		if(select(FD_SETSIZE, &wip, NULL, NULL, NULL) < 0) 
		{
			perror("Failure when doing select");
			exit(EXIT_FAILURE);
		}
		// Handle when a socket has something to do
		for(int i = 0; i < FD_SETSIZE; ++i) 
		{
			memset(msg, 0, MSG_MAX);
			if(FD_ISSET(i, &wip)) {
				// Handle new client wanting to connect
				if(i == sfd) {
					/* Accept a new connection request. */
					c_len = sizeof(cli_addr);
					new_sfd = accept(sfd, (struct sockaddr *) &cli_addr, &c_len);
					if(new_sfd < 0) {
						perror("server: accept error");
						return(1);
					}

					/* Add the file descriptor to the list of file descriptors */
					FD_SET(new_sfd, &master);
				}
				// Handle client sending message
				else {
					recv(i, msg, MSG_MAX, 0);

					// Approve client name
					if(strstr(msg, "Name: "))
					{
						bool name_used = false;

						// Parse out name
						char name[NAME_MAX];
						memcpy(name, &msg[6], NAME_MAX);
						for(int v = 0; v < NAME_MAX; v++) {
							if(name[i] == '\n') {
								memset(&name[i], 0, 1);
							}
						}

						// Check if the name is already used
						for(int q = 0; q < num_names; q++) {
							if(strncmp(name, names[q], NAME_MAX) == 0)
							{
								name_used = true;
							}
						}

						// Handle whether or not the name is already used
						if(name_used) {
							// Send verification to client
							send(i, "bad", MSG_MAX, 0);
						}
						else {
							// Send verification to client
							if(num_names == 0) {
								send(i, "good; You are the first user to join the chat.", MSG_MAX, 0);
							}
							else {
								send(i, "good", MSG_MAX, 0);
							}

							// Add name to list of names
							strcpy(names[num_names], name);
							num_names++;

							// Tell all current users about new user
							for(int w = 0; w < FD_SETSIZE; ++w) {
								if((w != i) && (w != sfd)) {
									char newClientAnnouncement[MSG_MAX];
									memset(newClientAnnouncement, 0, MSG_MAX);
									snprintf(newClientAnnouncement, MSG_MAX, "%s has joined the chat.\n", name);
									send(w, newClientAnnouncement, MSG_MAX, 0);
								}
							}
						}

						continue;
					}
					// Remove client
					if(strstr(msg, "quit"))
					{
						close(i);

						// Remove the fd from the master set
						FD_CLR(i, &master);
					}
					// Forward message
					for(int j = 0; j < FD_SETSIZE; ++j) {
						if((j != i) && (j != sfd)) {
							send(j, msg, MSG_MAX, 0);
						}
					}
				}	
			}
		}
	}
}