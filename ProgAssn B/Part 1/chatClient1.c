/*
Client1

References:
	https://therighttutorial.wordpress.com/2014/06/09/multi-client-server-chat-application-using-socket-programming-tcp/
	https://bitbucket.org/vidyakv/network-programming/commits/7f19f9145ab9
	https://github.com/rtroxler/C-Chatroom/blob/master/Part1/client.c
	https://github.com/jhauserw3241/CIS525-Networking/blob/master/ProgAssign2/part1/client.c
	
	I referenced the 2 githubs the most out of the 4 resources above. I used the other 2 to modify the code so 
	i could further improve on socket, file descriptor, and tcp usage.
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
#include "inet.h"

#define NAME_MAX 20
#define MSG_MAX 1000

int sfd;
char name[NAME_MAX];

int main(int args, char **argv)
{
	struct sockaddr_in s_addr;
	char msg[MSG_MAX];//array to hold message
	int nread; //num of chars of input stream
	fd_set master, wip;

	//Initialize the 2 sockets
	FD_ZERO(&master);
	FD_ZERO(&wip);

	/* Set up the address of the server to be contacted. */
	memset((char *) &s_addr, 0, sizeof(s_addr));
	
	/*Referenced this from wordpress code*/
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	s_addr.sin_port	= htons(SERV_TCP_PORT);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	connect(sfd, (struct sockaddr *) &s_addr, sizeof(s_addr));
	
	/* Add file descriptors */
	FD_SET(STDIN_FILENO, &master);
	FD_SET(sfd, &master);

	printf("Server connection successful.\n");

	bool nameApproved = false;
	while(nameApproved != true) 
	{
		// Get name from user
		printf("Your name: ");
	  fgets(name, NAME_MAX, stdin);
		for(int i = 0; i < NAME_MAX; i++) 
		{
			if(name[i] == '\n') {
				memset(&name[i], 0, 1);
			}
		}
		// Compare name so that it wont confuse server
		if(strcmp(name, "Name") == 0) 
		{
			printf("Choose a different name.\n");
			continue;
		}

		//Message prep for server
		char name_msg[6 + NAME_MAX];
		memset(name_msg, 0, 6 + NAME_MAX);
		snprintf(name_msg, 6 + NAME_MAX, "Name: %s\n", name);

		// Send the user's name to the server.
		write(sfd, name_msg, 6 + NAME_MAX);
		memset(msg, 0, MSG_MAX);
		nread = read(sfd, msg, MSG_MAX);
		
		if(nread <= 0) printf("Nothing read, nread = %d.\n", nread);
		
		//appends good, saying the name has been approved to join server
		if(strstr(msg, "good")) 
		{
			nameApproved = true;

			if(strstr(msg, "good; ")) 
			{
				char firstPersonMsg[MSG_MAX];
				memcpy(firstPersonMsg, &msg[6], MSG_MAX);
				printf("%s\n", firstPersonMsg);
			}
		}
		else 
		{
			printf("That name is already taken. Please choose a different name.\n");
		}
	}

	//Begin loop to allow others to chat
	for(;;) 
	{
		wip = master;

		// Wait for activity on file descriptor
		if(select(FD_SETSIZE, &wip, NULL, NULL, NULL) < 0) 
		{
			perror("select: failure");
			exit(EXIT_FAILURE);
		}

		//Handler for when the FD has activity
		for(int i = 0; i < FD_SETSIZE; ++i) 
		{
			if(FD_ISSET(i, &wip)) 
			{
				//Handler for user input
				if(i == STDIN_FILENO) 
				{
					memset(msg, 0, MSG_MAX);
					fgets(msg, MSG_MAX, stdin);
					msg[strcspn(msg, "\n")] = 0;

					//Handler for unallowed message(s)
					if(strcmp(msg, "quit") == 0)
					{
						printf("That message is not allowed\n");
					}
					// Send message to server
					else 
					{
						//store message in temp array for transport
						char temp[NAME_MAX + 2 + MSG_MAX];
						memset(temp, 0, NAME_MAX + 2 + MSG_MAX);
						snprintf(temp, NAME_MAX + 2 + MSG_MAX, "%s: %s\n", name, msg);//Actual sending of message to server
						write(sfd, temp, MSG_MAX);
					}
				}
				//Handler of incoming messages from server
				else 
				{
					nread = read(sfd, msg, MSG_MAX);
					if(nread > 0) 
					{
						printf("%s", msg);
					} else printf("Nothing read, nread = %d.\n", nread);			
				}
			}
		}

	}
	close(sfd);//make sure to close the FD
	return(0);
}