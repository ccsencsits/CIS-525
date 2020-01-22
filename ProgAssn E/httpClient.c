/*----------------------------------------------------------*/
/* client.c - sample time/date client.                      */
/* Prog Assn E
/* CIS 525: Intro to Network Programming
/* References:
	https://www.w3schools.com/tags/ref_httpmethods.asp
	https://github.com/YongHaoWu/http_client
	https://github.com/jhauserw3241/CIS525-Networking/tree/master/ProgAssign3
	https://github.com/pradyuman/socket-c/blob/master/httpclient.c
/*----------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/inet.h>

#define IP_MAX 150
#define MSG_MAX 1000

int	serv_fd;
int main(int argc, char *argv[])
{
	struct sockaddr_in serv_addr;
	int interact_status, nread, serv_port = 80; 
	char serv_ip[IP_MAX],  page[MSG_MAX];


	/*********************************
	* Begin ERROR Checking for inputs*
	**********************************/
	// Verify that number of args are correct. 
	if((argc < 2) || (argc > 3))
	{
		printf("Too few or too many args. Need website and port number (otional).\n");
		return(1);
	}
	
	/********************************************** 
	* Handle getting IP from hostname through DNS *
	***********************************************/		
	struct hostent *hp = gethostbyname(argv[1]);
	if(hp == NULL)
	{
		printf("gethostbyname() failed\n");
		return(1);
	}
    /*********************
    * End ERROR Checking *
    **********************/
	unsigned int i = 0;
	while(hp->h_addr_list[i] != NULL)
	{
		i++;
	}
	
	memset(serv_ip, 0, IP_MAX);
	snprintf(serv_ip, IP_MAX, "%s", inet_ntoa(*(struct in_addr*)(hp->h_addr_list[0])));

	/******************
	* Get PORT NUMBER *
	*******************/
	if(argc == 3)
	{
		serv_port = atoi(argv[2]);
		if((serv_port < 0) || (serv_port >= 65535))
		{
			printf("Invalid port number! Range is 0 - 65534.\n");
			return(1);
		}
	}
	printf("Trying %s on port %d...\n", serv_ip, serv_port);

	/********************************************************
	* Get specific subpage of website.  		            *
	* OPTIONAL: can hit enter to skip and stay on main page *
	*********************************************************/		
	printf("Which page? (e.g. '/research/' OR hit enter for no page.)\n");
	
	/**********************
	* Set memory for page *
	***********************/
	memset(page, 0, MSG_MAX);
	fgets(page, MSG_MAX, stdin);
	for(int i = 0; i < MSG_MAX; i++)
	{
		if(page[i] == '\n')
		{
			memset(&page[i], 0, 1);
		}
	}

	/**************************************************** 
	* Set up the address of the server to be contacted. *
	* Refernced Eugene's example of net1
	*****************************************************/
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family			= AF_INET;
	serv_addr.sin_addr.s_addr	= inet_addr(serv_ip);
	serv_addr.sin_port				= htons(serv_port);

	/**************************************************** 
	* Create a socket (an endpoint for communication). *
	* Refernced Eugene's example of net1
	*****************************************************/
	if((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	  printf("[ERROR]: Cannot Open Stream Socket\n");
		return(1);
	}

	/********************************** 
	* Handle conecting to the server. *
	***********************************/	
	if(connect(serv_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	  printf("[ERROR]: Cannot Connect to Server\n");
		return(1);
	}

	printf("Connected to %s.\n\n", hp->h_name);
	
	/**********************************
	* Handle the GET request for HTTP *
	***********************************/	
	char request[MSG_MAX + 4];
	memset(request, 0, MSG_MAX + 4);
	snprintf(request, MSG_MAX + 4, "GET %s HTTP/1.0\r\n\r\n", page);
	write(serv_fd, request, MSG_MAX);

	// Handle message from server
	int content_size;
	char *content = malloc(content_size);

	char temp[MSG_MAX];
	memset(temp, 0, MSG_MAX);
	
	while(read(serv_fd, temp, MSG_MAX) != 0)
	{
		content_size = content_size + MSG_MAX;
		char *content_switch = realloc(content, content_size);
		strcpy(content_switch, content);
		strcat(content_switch, temp);

		memset(temp, 0, MSG_MAX);
	}

	char status[MSG_MAX];
	memset(status, 0, MSG_MAX);
	int statusSize = -1;
	
	for(int j = 0; j < MSG_MAX; j++)
	{
		if(content[j] == '\n')
		{
			statusSize = j;
			break;
		}
	}
	snprintf(status, statusSize, "%s", content);
	
	/******************************************** 
	* Handle ERROR from no "HTTP/1.0(1) 200 OK' *
	*********************************************/	
	if((strcmp(status, "HTTP/1.0 200 OK") != 0) && (strcmp(status, "HTTP/1.1 200 OK") != 0))
	{
		printf("%s\n", status);
	}
	/********************************** 
	* Handle the "HTTP/1.0(1) 200 OK' *
	***********************************/		
	else {
		char noHeader[content_size];
		memset(noHeader, 0, content_size);
		char *headerEnd = strstr(content, "\r\n\r\n");
		int posHeader = (headerEnd - content) + 4;
		
		/********************************************
		* loop through header of the HTML/HTTP 'OK' *
		*********************************************/
		for(int k = posHeader; k < content_size; k++)
		{
			noHeader[k - posHeader] = content[k];
		}

		printf("%s\n", noHeader);
	}
	close(serv_fd);
	return(0);
}