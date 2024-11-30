#include "proxy_parse.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winsock.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>



#define MAX_CLIENTS 5
#define MAX_BYTES 10*(1<<10) // 10KB
#define MAX_CACHE_ELEMENT_SIZE 10*(1<<10)// 10KB
#define  MAX_CACHE_SIZE 20*(1<<20)// 20MB

typedef struct cache_entry cache_entry;

struct cache_entry {
    char* data;
    int len;
    char* url;
    time_t timestamp;
    struct cache_entry* next;
};

cache_entry* find_cache_entry(char *url );
int add_cache_entry(char* url, char* data, int size);
void remove_cache_entry();


int port_number = 8080;
int proxy_socket_id;

pthread_t threads_ids[MAX_CLIENTS]; // store thread ids
sem_t semaphore; // semaphore for thread synchronization act just like a mutex but with multiple values 
// sem_wait(sem_t *sem) decrements the value of the semaphore pointed to by sem. If the value of the semaphore is greater than zero, the decrement proceeds, and the function returns, immediately. If the semaphore currently has the value zero, then the call blocks until either it becomes possible to perform the decrement (i.e., the semaphore value rises above zero), or a signal handler interrupts the call.
// sem_post(sem_t *sem) increments the value of the semaphore pointed to by sem. If the value of the semaphore is currently zero, then a process attempting to decrement it waits until the value becomes greater than zero.
// sem_init(sem_t *sem, int pshared, unsigned int value) initializes the unnamed semaphore at the address pointed to by sem. The value argument specifies the initial value for the semaphore. The pshared argument indicates whether this semaphore is to be shared between the threads of a process, or between processes. If pshared has the value 0, then the semaphore is shared between threads of the process. If pshared is nonzero, then the semaphore is shared between processes. Only semaphores that are shared between processes should be placed in shared memory.
// sem_destroy(sem_t *sem) destroys the unnamed semaphore at the address pointed to by sem. The semaphore should be destroyed by the process that created it, after the semaphore is no longer needed. No threads should be blocked waiting on the semaphore when it is destroyed.
// sem_signal(sem_t *sem) increments the value of the semaphore pointed to by sem. If the value of the semaphore is currently zero, then a process attempting to decrement it waits until the value becomes greater than zero.

pthread_mutex_t lock; // mutex for cache race condition
// pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) initializes the mutex object pointed to by mutex with attributes specified by attr. If attr is NULL, the default mutex attributes are used. The mutex is created in an unlocked state.
// pthread_mutex_lock(pthread_mutex_t *mutex) locks the mutex object pointed to by mutex. If the mutex is already locked, the calling thread
// pthread_mutex_unlock(pthread_mutex_t *mutex) unlocks the mutex object pointed to by mutex. If there are threads blocked on the mutex object referenced by mutex when pthread_mutex_unlock() is called, resulting in the mutex becoming available, the scheduling policy shall determine which thread shall acquire the mutex.
// pthread_mutex_destroy(pthread_mutex_t *mutex) destroys the mutex object referenced by mutex; the mutex object becomes, in effect, uninitialized. An implementation may cause pthread_mutex_destroy() to set the object referenced by mutex to an invalid value. A destroyed mutex object can be reinitialized using pthread_mutex_init(); the results of otherwise referencing the object after it has been destroyed are undefined.
// pthread_mutex_trylock(pthread_mutex_t *mutex) tries to lock the mutex object pointed to by mutex. If the mutex is already locked, the call returns immediately. If the mutex is currently unlocked, it becomes locked and the call returns with the mutex object referenced by mutex in the locked state with the calling thread as its owner.

cache_entry* cache_head = NULL; // head of the cache linked list
int cache_size = 0; // size of the cache


int connect_remote_server(char *host_addr, int port_num){
    int remote_socket_id = socket(AF_INET, SOCK_STREAM, 0); // create a socket for the remote server
    if(remote_socket_id < 0){ // check if the socket was created successfully
        perror("Error creating socket"); // print error message
        return -1; // return error
    }
    struct hostent *server_host = gethostbyname(host_addr); // get the host by name
    if(server_host == NULL){ // check if the host was found
        perror("Error finding host"); // print error message
        return -1; // return error
    } 
   
   struct sockaddr_in server_addr; // create a socket address for the server    
//    bzero((char *) &server_addr, sizeof(server_addr)); // set the server address to 0
    memset(&server_addr, 0, sizeof(server_addr)); // instead of bzero
   server_addr.sin_family = AF_INET; // set the address family to IPv4
   server_addr.sin_port = htons(port_num); // set the port number to network byte order
    // bcopy((char *)server_host->h_addr, (char *)&server_addr.sin_addr.s_addr, server_host->h_length); // copy the server address to the socket address 
    memcpy(&server_addr.sin_addr.s_addr, server_host->h_addr, server_host->h_length);

    if(connect(remote_socket_id, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){ // connect to the remote server
        perror("Error connecting to server"); // print error message
        return -1; // return error
    }
    return remote_socket_id; // return the remote socket id
}


int handle_request(int client_socket_id,  ParsedRequest *request, char *temp_request){
    char *buf = (char *)malloc(MAX_BYTES*sizeof(char));
    strcpy(buf, "GET");
    strcat(buf,request->path);
    strcat(buf," ");
    strcat(buf,request->version);
    strcat(buf,"\r\n");
    size_t len = strlen(buf);
    printf("Request: %s\n", buf);
    
    if(ParsedHeader_set(request, "Connection", "close") < 0){
        printf("Error setting connection header\n");
        // return -1;
    }

    if(ParsedHeader_get(request, "Host") == NULL){
        printf("Host header not found setting host header\n");
       if(ParsedHeader_set(request, "Host",  request->host) < 0){
           printf("Error setting host header\n");
        //    return -1;
       }
    }

    if(ParsedRequest_unparse(request, buf + len, MAX_BYTES - len) < 0){
        printf("Error unparsing request\n");
        // return -1;
    }

    int server_port = 80;
    if(request->port != NULL){
        server_port = atoi(request->port);
    }

    int remote_socket_id = connect_remote_server(request->host, server_port);
    if(remote_socket_id < 0){
        printf("Error connecting to remote server\n");
        return -1;
    }

    int bytes_sent_to_server = send(remote_socket_id, buf, strlen(buf), 0);
    // bzero(buf, MAX_BYTES);
    memset(buf, 0, MAX_BYTES);                  

    if(bytes_sent_to_server < 0){
        perror("Error sending message to server");
        return -1;
    }
    bytes_sent_to_server = recv(remote_socket_id, buf, MAX_BYTES-1, 0);
    if(bytes_sent_to_server < 0){
        perror("Error receiving message from server");
        return -1;
    }
    char *temp_buffer = (char *)malloc(MAX_BYTES*sizeof(char));
    // bzero(temp_buffer, MAX_BYTES);
    memset(temp_buffer, 0, MAX_BYTES);
    int temp_buffer_size = MAX_BYTES;
    int temp_buffer_index = 0;

    while(bytes_sent_to_server > 0){
        bytes_sent_to_server = send(client_socket_id, buf, bytes_sent_to_server, 0);
        if(bytes_sent_to_server < 0){
            perror("Error sending message to client");
            return -1;
        }

        for(size_t i = 0; i < sizeof(bytes_sent_to_server)/sizeof(char); i++){
            temp_buffer[temp_buffer_index] = buf[i];
            temp_buffer_index++;
            if(temp_buffer_index == temp_buffer_size){
                temp_buffer_size *= 2;
                temp_buffer = (char *)realloc(temp_buffer, temp_buffer_size*sizeof(char));
                if(bytes_sent_to_server < 0){
                    perror("Error reallocating memory");
                    break;
                }
                // todo
                // bzero(temp_buffer + temp_buffer_index, temp_buffer_size - temp_buffer_index); // set the buffer to 0 + temp_buffer_index is used to set the buffer to 0 from the index to the end of the buffer
                memset(temp_buffer + temp_buffer_index, 0, temp_buffer_size - temp_buffer_index);
                bytes_sent_to_server = recv(remote_socket_id, buf, MAX_BYTES-1, 0);
            }
            temp_buffer[temp_buffer_index] = '\0';
            free(buf);
            add_cache_entry(temp_request,temp_buffer,strlen(temp_buffer));
            free(temp_buffer);
            close(remote_socket_id);
            return 0;
        }
    }
}

int checkHTTPversion(char *version){
    if(strcmp(version, "HTTP/1.0") == 0 || strcmp(version, "HTTP/1.1") == 0){
        return 1;
    }
    return -1;
}

int sendErrorMessage(int socket, int status_code)
{
	char str[1024];
	char currentTime[50];
	time_t now = time(0);

	struct tm data = *gmtime(&now);
	strftime(currentTime,sizeof(currentTime),"%a, %d %b %Y %H:%M:%S %Z", &data);

	switch(status_code)
	{
		case 400: snprintf(str, sizeof(str), "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
				  printf("400 Bad Request\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 403: snprintf(str, sizeof(str), "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
				  printf("403 Forbidden\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 404: snprintf(str, sizeof(str), "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
				  printf("404 Not Found\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 500: snprintf(str, sizeof(str), "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
				  //printf("500 Internal Server Error\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 501: snprintf(str, sizeof(str), "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
				  printf("501 Not Implemented\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 505: snprintf(str, sizeof(str), "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
				  printf("505 HTTP Version Not Supported\n");
				  send(socket, str, strlen(str), 0);
				  break;

		default:  return -1;

	}
	return 1;
}


void *handle_client_thread_fn(void *new_socket){ 
    sem_wait(&semaphore); // wait for the semaphore to be available or decrement the semaphore value by 1 or block the thread if the semaphore value is 0
    int p;
    sem_getvalue(&semaphore, &p); // get the value of the semaphore
    printf("Semaphore value: %d\n", p); // print the semaphore value
    int *client_socket_id = (int *)new_socket;  // get the client socket id
    int socket_id = *client_socket_id; // get the client socket id
    int bytes_sent_by_client, len; // variables to store the number of bytes sent by the client and the length of the message
    char *buffer = (char *)calloc(MAX_BYTES,sizeof(char)); // allocate memory for the buffer to store the message from the client , calloc() is used to allocate memory in the heap and initialize it to 0 while malloc() is used to allocate memory in the heap but does not initialize it
    // bzero(buffer, MAX_BYTES); // set the buffer to 0
    memset(buffer, 0, MAX_BYTES); // instead of bzero
    bytes_sent_by_client = recv(socket_id, buffer, MAX_BYTES, 0); // receive the message from the client and store it in the buffer , recv() is used to receive messages from the client , the third argument is the maximum number of bytes to receive and the fourth argument is the flags to make the call blocking or non-blocking? (not sure)
    if(bytes_sent_by_client < 0){ // check if the message was received successfully
        perror("Error receiving message from client"); // print error message
        exit(1); // exit with error
    }
    while(bytes_sent_by_client > 0){ // loop while the number of bytes sent by the client is greater than 0
        len = strlen(buffer); // get the length of the message
        printf("Received message from client: %s\n", buffer); // print the message received from the client
        if(strstr(buffer, "\r\n\r\n") == NULL){ // check if the message contains the end of the message
            bytes_sent_by_client = recv(socket_id, buffer + len, MAX_BYTES - len, 0); // receive the rest of the message from the client
            if(bytes_sent_by_client < 0){ // check if the message was received successfully
                perror("Error receiving message from client"); // print error message
                exit(1); // exit with error
            }
        }
        else{
            break; // break the loop
        }
    }

    // make copy of request to store in cache
    char *temp_request = (char *)malloc(strlen(buffer)*sizeof(char)+1); // allocate memory for the temporary request using malloc() , malloc() is used to allocate memory in the heap but does not initialize it 
    // strcpy(temp_request, buffer); // copy the request to the temporary request
    for(size_t i = 0; i < strlen(buffer); i++){
        temp_request[i] = buffer[i];
    }
    temp_request[strlen(buffer)] = '\0'; // set the last character to null
    printf("Request: %s\n", temp_request); // print the request
    

    struct cache_entry *temp_entry = find_cache_entry(temp_request); // find the cache entry in the cache linked list
    if(temp_entry != NULL){ // check if the cache entry was found
        printf("Cache hit\n"); // print message
        int size = temp_entry->len/sizeof(char); // get the size of the cache entry 
        int pos = 0; // initialize the position
        char *response = (char *)malloc(size*sizeof(char)); // allocate memory for the response using malloc()
        while(pos < size){ // loop while the position is less than the size
        //    bzero(response, size); // set the response to 0
          memset(response, 0, size); 
           for(int i = 0; i < size; i++){ // loop through the size
               response[i] = temp_entry->data[i]; // copy the data to the response
               pos++; // increment the position
           }
           send(socket_id, response, size, 0); // send the response to the client
        }
        printf("data retrieved from cache\n"); // print message
        printf("%s\n\n", response); // print the response
        free(response); // free the response
    }else if(bytes_sent_by_client > 0){
        printf("Cache miss\n"); 
        len = strlen(buffer); // get the length of the message

        // parse the request
        ParsedRequest *request = ParsedRequest_create(); // create a parsed request

        if(ParsedRequest_parse(request, buffer, len) < 0){ // parse the request
            printf("Error parsing request\n"); // print error message
            ParsedRequest_destroy(request); // destroy the parsed request
            free(buffer); // free the buffer
            close(socket_id); // close the socket
            sem_post(&semaphore); // increment the semaphore value by 1
            pthread_exit(NULL); // exit the thread
        }else{
            // bzero(buffer, MAX_BYTES); // set the buffer to 0
            memset(buffer, 0, MAX_BYTES); 
            // ParsedHeader_set(request, "Connection", "close"); // set the connection header to close
            // ParsedHeader_set(request, "Proxy-Connection", "close"); // set the proxy connection header to close
            // len = ParsedRequest_totalLen(request); // get the total length of the request
            // ParsedRequest_unparse(request, buffer, MAX_BYTES); // unparse the request
            // ParsedRequest_destroy(request); // destroy the parsed request
            // printf("Sending request to server: %s\n", buffer); // print the request
            // int server_socket_id = socket(AF_INET, SOCK_STREAM, 0); // create a socket for the server
            // if(server_socket_id < 0){ // check if the socket was created successfully
            //     perror("Error creating socket"); // print error message
            //     exit(1); // exit with error
            // }
            if(!strcmp(request->method, "GET")){ // check if the request method is GET
                printf("GET request\n"); // print message
                if(request->host && request->path && checkHTTPversion(request->version) == 1){
                    printf("Valid request\n"); 
                    bytes_sent_by_client = handle_request(socket, request, temp_request); // handle the request
                    if(bytes_sent_by_client == -1){ // check if the request was handled successfully
                        perror("Error handling request"); // print error message
                        sendErrorMessage(socket_id, 500); // send an error message to the client
                    }else{
                        sendErrorMessage(socket_id, 500); // send a success message to the client
                    }
                }else{
                    printf("only support GET, Invalid request\n"); 
                    sendErrorMessage(socket_id, 400); // send an error message to the client
                }
            }

        }
        ParsedRequest_destroy(request); // destroy the parsed request
    }else if(bytes_sent_by_client == 0){
        printf("Client disconnected\n"); // print message
    }
    shutdown(socket_id, SD_BOTH); // shut down the socket for reading and writing 
    close(socket_id); // close the socket
    sem_post(&semaphore); // increment the semaphore value by 1
    sem_getvalue(&semaphore, &p); // get the value of the semaphore
    printf("Semaphore post value: %d\n", p); // print the semaphore value
    free(buffer); // free the buffer
    free(temp_request); // free the temporary request
    return NULL; // return null

}


int main(int argc, char* argv[]){
    int client_socket_id, client_len;
    struct sockaddr_in client_addr,server_addr;
    sem_init(&semaphore, 0, MAX_CLIENTS);
    pthread_mutex_init(&lock, NULL);
    if(argc < 2){  // check if the port number is provided
        printf("Usage: %s <port_number>\n", argv[0]); // print usage
        return 1; // return error
    }
    if(argc == 2){ // check if the port number is provided
        port_number = atoi(argv[1]); // convert the port number to integer
    }else{
        printf("Expected 2 arguments, got %d\n", argc); // print error message
        exit(1); // exit with error
    }
    
    printf("Starting proxy server on port %d\n", port_number); // print message
    proxy_socket_id = socket(AF_INET, SOCK_STREAM, 0); // create a socket for the proxy server 
    // AF_INET is the address family for IPv4. SOCK_STREAM is the socket type for TCP. The third argument is set to 0 to use the default protocol, which is TCP.
    if(proxy_socket_id < 0){ // check if the socket was created successfully
        perror("Error creating socket"); // print error message
        exit(1); // exit with error
    }

    int reuse = 1; // set the reuse variable to 1   
    if(setsockopt(proxy_socket_id, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){ // set the socket options
    // The setsockopt() function is used to set options associated with a socket. The SOL_SOCKET argument specifies the level at which the option is defined. The SO_REUSEADDR option allows the socket to be bound to an address that is already in use.
        perror("Error setting socket options"); // print error message
        exit(1); // exit with error
    }

    // bzero((char *) &server_addr, sizeof(server_addr)); // set the server address to 0 , bzero() is used to set  all the bytes of the server_addr to 0
    memset(&server_addr, 0, sizeof(server_addr));  // instead of bzero

    server_addr.sin_family = AF_INET; // set the address family to IPv4
    server_addr.sin_port = htons(port_number); // set the port number, htons() is used to convert the port number to network byte order for example 8080 to 0x1F90
    server_addr.sin_addr.s_addr = INADDR_ANY; // set the address to INADDR_ANY which is the address of the local host
    if(bind(proxy_socket_id, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){ // bind the socket to the server address to start listening for connections 
        perror("Error binding socket, port is not available"); // print error message
        exit(1); // exit with error
    }
    printf("Proxy server binded on port %d\n", port_number);
    int listen_status = listen(proxy_socket_id, MAX_CLIENTS );  // start listening for connections on the socket, the second argument is the maximum number of connections that can be queued
    if(listen_status < 0){ // check if the listening was successful
        perror("Error listening on socket"); // print error message
        exit(1); // exit with error
    }
    printf("Proxy server listening on port %d\n", port_number); // print message
    int i = 0; // initialize the counter
    int connected_socket_ids[MAX_CLIENTS]; // store the connected socket ids
    while(1){ // loop forever
        // bzero((char *) &client_addr, sizeof(client_addr)); // set the client address to 0
        memset(&client_addr, 0, sizeof(client_addr));
        client_len = sizeof(client_addr); // set the client length
        client_socket_id = accept(proxy_socket_id, (struct sockaddr *) &client_addr, &client_len); // accept the connection from the client and get the client socket id
        if(client_socket_id < 0){ // check if the connection was successful
            perror("Error accepting connection"); // print error message
            exit(1); // exit with error
        }else{
            printf("Accepted connection from %s\n", inet_ntoa(client_addr.sin_addr)); // print message
            connected_socket_ids[i] = client_socket_id; // store the client socket id   
        }

        struct sockaddr_in *client_addr_copy = (struct sockaddr_in *)&client_addr; // copy the client address
        struct in_addr ip_addr = client_addr_copy->sin_addr; // get the ip address of the client 
        char str[INET_ADDRSTRLEN]; // create a string to store the ip address
        inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN); // convert the ip address to a string and store it in str
        printf("Client is Connected With Port %d and IP %s\n", ntohs(client_addr_copy->sin_port), str); // print the client port and ip address

        pthread_create(&threads_ids[i], NULL, handle_client_thread_fn, (void *)&connected_socket_ids[i]); // create a thread to handle the client connection  handle_client_thread_fn is the function that will be executed by the thread after it is created and  (void *)&connected_socket_ids[i] is the argument to the function , NULL is the default thread attributes
        i++; // increment the counter
    }

    close(client_socket_id); // close the client socket
    return 0; // return success
}


// Function to remove a cache entry from the cache linked list
void remove_cache_entry(){
    cache_entry* previous;
    cache_entry* current;
    cache_entry* temp;
    int temp_mutex_lock = pthread_mutex_lock(&lock); // lock the mutex
    printf("cache remove Mutex lock value: %d\n", temp_mutex_lock); // print the mutex lock value
    if(cache_head != NULL){
        for(current = cache_head, previous=cache_head, temp = cache_head; current->next != NULL; current = current->next){
            if(current->next->timestamp < temp->timestamp){
                temp = current->next;
                previous = current;
            }
        }
        if(temp == cache_head){
            cache_head = cache_head->next;
        }else{
            previous->next = temp->next;
        }
        cache_size -= temp->len + strlen(temp->url) + sizeof(cache_entry)+1;
        free(temp->data);
        free(temp->url);
        free(temp);
    }
    int temp_mutex_unlock = pthread_mutex_unlock(&lock); // unlock the mutex
    printf("cache remove Mutex unlock value: %d\n", temp_mutex_unlock); // print the mutex unlock value
}


// Function to find a cache entry in the cache linked list
cache_entry* find_cache_entry(char* url){
    cache_entry* current = NULL; // create a current cache entry
    int temp_mutex_lock = pthread_mutex_lock(&lock); // lock the mutex to prevent 
    printf("Mutex lock value: %d\n", temp_mutex_lock); // print the mutex lock value 
    if(cache_head != NULL){
        current = cache_head; // set the current cache entry to the head of the cache linked list
        while(current != NULL){ 
            if(strcmp(current->url, url) == 0){ // check if the url of the current cache entry is the same as the url
                printf("timestamp before: %ld\n", current->timestamp); // print the  time
                printf("\n Url found in cache\n"); // print message
                current->timestamp = time(NULL); 
                printf("timestamp after: %ld\n", current->timestamp);
                break;
            }
        current = current->next; // set the current cache entry to the next cache entry
        }
    }else{
        printf("Url not found in cache\n");
    }
    int temp_mutex_unlock = pthread_mutex_unlock(&lock); // unlock the mutex
    printf("Mutex unlock value: %d\n", temp_mutex_unlock); 
    return current; 
}

// Function to add a cache entry to the cache linked list
int add_cache_entry(char *url, char *data, int size){
    int temp_mutex_lock = pthread_mutex_lock(&lock);
    printf("Mutex lock value: %d\n", temp_mutex_lock);
    int element_size = size+1+strlen(url)+sizeof(cache_entry); // calculate the size of the cache entry
    if(element_size > MAX_CACHE_ELEMENT_SIZE){
        printf("Cache size exceeded\n");
        temp_mutex_lock = pthread_mutex_unlock(&lock);
        return 0;
    }else{
         while(cache_size + element_size > MAX_CACHE_SIZE){
            remove_cache_entry();
    }
    cache_entry* new_entry = (cache_entry *)malloc(sizeof(cache_entry)); // allocate memory for the new cache entry
    // todo
    new_entry->data = (char *)malloc((size+1)*sizeof(char)); // allocate memory for the data in the cache entry
    strcpy(new_entry->data, data); // copy the data to the cache entry 
    new_entry->url = (char *)malloc((strlen(url)+1)*sizeof(char)); // allocate memory for the url in the cache entry
    strcpy(new_entry->url, url); // copy the url to the cache entry
    new_entry->timestamp = time(NULL); // set the timestamp to the current time
    new_entry->len = size; // set the length of the cache entry to 
    new_entry->next = cache_head; // set the next cache entry to the head of the cache linked list
    cache_head = new_entry; // set the head of the cache linked list to the new cache entry
    cache_size += element_size; // increment the cache size by the size of the new cache entry
    temp_mutex_lock = pthread_mutex_unlock(&lock); // unlock the mutex
    printf("add cache Mutex unlock value: %d\n", temp_mutex_lock); // print the mutex unlock value
    return 1; // return success
    }
    return 0; // return error
}