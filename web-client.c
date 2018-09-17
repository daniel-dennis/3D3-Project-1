#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <fcntl.h>
#include <unistd.h>

#define STR_LEN 1048576
#define URL_LEN 256
#define HTTP_VERSION "HTTP/1.0"
#define HTTP_METHOD "GET"

void parser(char* address, char* port, char* file, char* argv);
void save_file(char* name, char* buffer);

int main(int argc, char* argv[])
{
	int i, j;
	
	/* Setup */
	char buffer[STR_LEN];
	char input[URL_LEN];
	char file[URL_LEN];
	char request[URL_LEN];
	char address[URL_LEN];
	char port[5];
	char delim[URL_LEN];
	
	for(i = 1; i < argc; i++)
	{
		/* Parse URL and build request */
		
		parser(&address[0], &port[0], &file[0], argv[i]);
		printf("Address: %s\n", address);
		printf("Port: %s\n", port);
		printf("file: %s\n", file);
		
		memset(request, 0, URL_LEN);
		strcat(request, HTTP_METHOD);
		strcat(request, " ");
		strcat(request, file);
		strcat(request, " ");
		strcat(request, HTTP_VERSION);
		strcat(request, "\r\n\r\n");
		printf("Request to be sent: %s", request);
		
		/* Begin setup of network socket */

		int info;
		struct addrinfo server_setup;
		struct addrinfo* server_status;
		memset(&server_setup, 0, sizeof(server_setup));
		
		server_setup.ai_family = AF_UNSPEC;
		server_setup.ai_socktype = SOCK_STREAM;
		
		info = getaddrinfo(address, port, &server_setup, &server_status);
		if(info)
		{
			perror("Address setup error");
			return 1;	
		}
		
		/* Socket */
		int client_socket;
		client_socket = socket(server_status->ai_family, server_status->ai_socktype, server_status->ai_protocol);
		if(client_socket == -1)
		{
			perror("Socket error");	
		}
		
		/* Connect */
		info = connect(client_socket, server_status->ai_addr, server_status->ai_addrlen);
		if(info)
		{
			perror("Connection error");	
		}
		
		/* Send */
		int len = strlen(request);
		info = send(client_socket, &request, len, 0);
		if(info == -1)
		{
			perror("Send");
		}
		
		
		/* Receive */
		info = recv(client_socket, &buffer, STR_LEN, 0);
		if(info == -1)
		{
			perror("Receive");	
		}
		
		/* Results */
		
		printf("~ Buffer ~\n%s\n", buffer);
		
		/* Store File */
		if(!strcmp(input, "/"))
		{
			strcpy(input, "index.html");	
		}
		save_file(file, &buffer[0]);
		
		close(client_socket);
	}
		
	return 0;
}

void save_file(char* name, char* buffer)
{
	char buffer_copy[STR_LEN];
	char name_copy[URL_LEN];
	strcpy(buffer_copy, buffer);
	strcpy(name_copy, name);
	
	strcpy(name_copy, "");
	strcat(&name_copy[0], name);
	
	char delim[2] = {'<', 0};
	char* str = strstr(&buffer_copy[0], &delim[0]);
	if(str)
	{
		FILE* file_to_save;
		file_to_save = fopen(name_copy, "w");
		if(!file_to_save)
		{
			perror("File error");
			printf("name_copy: %s\n", name_copy);
			return;	
		}
		fputs(str, file_to_save);
		fclose(file_to_save);
	}
	return;
}

void parser(char* address, char* port, char* file, char* argv)
{
		int j;
		char* s1 = argv;
		char* s2 = argv;
		s2 += 7;
		
		while(*s1 != '/')
		{
			s1++;	
		}
		s1 += 2;
		
		while(*s2 != ':')
		{
			s2++;	
		}
		
		for(j = 0; j < URL_LEN; j++)
		{
			*address = *s1;
			address++;
			s1++;
			if(s1 == s2)
			{
				address = 0;
				break;
			}
		}
		
		while(*s1 != '/')
		{
			s1++;	
		}
		s2++;
		
		for(j = 0; j < URL_LEN; j++)
		{
			*port = *s2;
			port++;
			s2++;
			if(s1 == s2)
			{
				*port = 0;
				break;
			}
		}
		s2++;
		strcpy(file, s2);
	return;
}
