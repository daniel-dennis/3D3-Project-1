/*
	It has occured to me that it would have been better writing this in C++
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>

#define NUM_THREADS 5
#define QUEUE_LENGTH 10

#define STR_LEN 4096
#define SHORT_STR 128
#define HTTP_VERSION_1_0 "HTTP/1.0"
#define HTTP_VERSION_1_1 "HTTP/1.1"
#define HTTP_GET "GET"

#define STATUS_200 "200 OK"
#define STATUS_201 "201 Created"
#define STATUS_202 "202 Accepted"
#define STATUS_204 "204 No Content"
#define STATUS_301 "301 Moved Permanently"
#define STATUS_302 "302 Moved Temporarily"
#define STATUS_304 "304 Not Modified"
#define STATUS_400 "400 Bad Request"
#define STATUS_401 "401 Unauthorized"
#define STATUS_403 "403 Forbidden"
#define STATUS_404 "404 Not Found"
#define STATUS_500 "500 Internal Server Error"
#define STATUS_501 "501 Not Implemented"
#define STATUS_502 "502 Bad Gateway"
#define STATUS_503 "503 Service Unavailable"

struct server_data
{
	struct addrinfo server_setup;
	struct addrinfo* server_status;
	int server_socket;
	char* file;
}sd;

void* request_handler(void* p);
void handler(char* request, char* response);

int main(int argc, char* argv[])
{
	char host_input[SHORT_STR];
	char port_input[SHORT_STR];
	
	strcpy(host_input, argv[1]);
	strcpy(port_input, argv[2]);
	
	sd.file = argv[3];
	
	int i, rc;
	int id[NUM_THREADS];
	pthread_t thread[NUM_THREADS];
	int info;
	
	printf("user inputs %s %s \n", host_input, port_input);

	memset(&sd.server_setup, 0, sizeof(sd.server_setup));
	
	sd.server_setup.ai_family = AF_UNSPEC;
	sd.server_setup.ai_socktype = SOCK_STREAM;
	sd.server_setup.ai_flags = AI_PASSIVE;
	
	info = getaddrinfo(host_input, port_input, &sd.server_setup, &sd.server_status);
	if(info)
	{
		perror("Address setup");
	}
	
	/* Socket */
	
	sd.server_socket = socket(sd.server_status->ai_family, sd.server_status->ai_socktype, sd.server_status->ai_protocol);
	
	/* Bind */
	
	info = bind(sd.server_socket, sd.server_status->ai_addr, sd.server_status->ai_addrlen);
	if(info)
	{
		perror("Bind");
		return 1;
	}
	
	/* Listen */
	
	info = listen(sd.server_socket, QUEUE_LENGTH);
	if(info)
	{
		perror("Listen");
		return 1;
	}
	
	for(i = 0; i < NUM_THREADS; i++)
	{
		id[i] = i;
		rc = pthread_create(&thread[i], NULL, request_handler, (void*)(id + i));
		if(rc)
		{
			printf("Error %d\n", i);	
		}
	}
	
	for(i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(thread[i], NULL);	
	}
	
	return 0;
}

void* request_handler(void* p)
{
	/* Declare variables */
	int* thread_no = (int*)p;
	int info;
	char buffer[STR_LEN];/* HTTP request from client stored here */
	char* buffer_p = &buffer[0];
	char http_response[STR_LEN];/* HTTP response to client stored here */
	char* http_response_p = &http_response[0];
	
	int client_socket;
	struct sockaddr_storage client_info;
	socklen_t client_info_size = sizeof(client_info);
	
	/* Accept and respond to requests */
	for(;;)
	{
		memset(http_response, 0, STR_LEN);
		memset(buffer, 0, STR_LEN);
		
		client_socket = accept(sd.server_socket, (struct sockaddr*)&client_info, &client_info_size);
		if(client_socket < 0)
		{
			perror("Accept");
		}
		
		/* Receive */
		info = recv(client_socket, &buffer, STR_LEN, 0);
		if(info < 0)
		{
			perror("Receive");
		}
		if(!strcmp(buffer, "HALT"))
		{
			break;	
		}
		printf("~\nThread %d ~ HTTP REQUEST\n%s\n~\n", *thread_no, buffer);
		
		/* Process Request */
		handler(buffer_p, http_response_p);
		printf("~\nHTTP RESPONSE\n%s\n~\n", http_response);
		
		/* Send */
		info = send(client_socket, &http_response, sizeof(http_response), 0);
		if(info < 0)
		{
			perror("Send");
		}
		
		close(client_socket);
	}
	printf("Thread %d exited\n", *thread_no);
	pthread_exit(NULL);
}

void handler(char* request, char* response)
{
	char dir_input[SHORT_STR];
	strcpy(dir_input, sd.file);
	
	char request_copy[STR_LEN];
	strcpy(request_copy, request);
	char* status = STATUS_200;
	char* str_ptr;
	char delim = ' ';
	int file_found = 1;
	FILE* input_file;
	memset(response, 0, STR_LEN);
	
	/*** Read in request ***/
	
	/* Check request type */
	str_ptr = strtok(request_copy, &delim);
	if(strcmp(str_ptr, HTTP_GET))
	{
		status = STATUS_400;
	}
		
	/* Check file */
	str_ptr = strtok(NULL, &delim);
	if(str_ptr[0] == '/')
	{
		str_ptr++;	
	}
	strcat(dir_input, "/");
	strcat(dir_input, str_ptr);
	char* in_ptr = &dir_input[1];
	if(*in_ptr == '/')
	{
		in_ptr++;
	}
	input_file = fopen(in_ptr, "r");
	if(!input_file)
	{
		perror("File");
		status = STATUS_404;
		file_found = 0;
	}
	printf("input_file: %s\n", in_ptr);
	
	/* Check HTTP version */
	delim = '\r';
	str_ptr = strtok(NULL, &delim);
	
	if(strcmp(str_ptr, HTTP_VERSION_1_0))
	{
		if(strcmp(str_ptr, HTTP_VERSION_1_1))
		{
			status = STATUS_501;
		}
	}
	
	/*** Build HTTP response ***/
	strcpy(response, HTTP_VERSION_1_0);
	strcat(response, " ");
	strcat(response, status);
	strcat(response, "\r\n\r\n");
	if(status == STATUS_200)
	{	
		char line[STR_LEN];
		while(fgets(line, STR_LEN, input_file))
		{
			strncat(response, line, STR_LEN);
		}
	}
	if(file_found)
	{
		fclose(input_file);	
	}
	
	return;
}