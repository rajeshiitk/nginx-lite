#include "proxy_parse.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_CLIENTS 100


struct cache_entry {
    char* data;
    int len;
    char* url;
    time_t timestamp;
    struct cache_entry* next;
};

cache_entry* find_cache_entry();
int add_cache_entry(char* url, char* data, int size);

void remove_cache_entry(cache_entry* entry);

int port_number = 8080;
int proxy_socket_id;

pthread_t threads_ids[MAX_CLIENTS]; // store thread ids
sem_t semaphore; // semaphore for thread synchronization act just like a mutex but with multiple values

pthread_mutex_t lock; // mutex for cache race condition


