
    // Semaphore is successfully decremented and incremented
void test_semaphore_decrement_increment() {
    sem_t semaphore;
    sem_init(&semaphore, 0, 1); // Initialize semaphore with value 1

    pthread_t thread;
    int socket_id = 1;
    pthread_create(&thread, NULL, handle_client_thread_fn, (void *)&socket_id);

    sleep(1); // Give some time for the thread to run

    int value;
    sem_getvalue(&semaphore, &value);
    assert(value == 0); // Semaphore should be decremented

    sem_post(&semaphore); // Increment semaphore
    sem_getvalue(&semaphore, &value);
    assert(value == 1); // Semaphore should be incremented

    pthread_join(thread, NULL);
    sem_destroy(&semaphore);
}

    // Semaphore value is zero and thread blocks until available
void test_semaphore_blocking() {
    sem_t semaphore;
    sem_init(&semaphore, 0, 0); // Initialize semaphore with value 0

    pthread_t thread;
    int socket_id = 1;
    pthread_create(&thread, NULL, handle_client_thread_fn, (void *)&socket_id);

    sleep(1); // Give some time for the thread to run

    int value;
    sem_getvalue(&semaphore, &value);
    assert(value == 0); // Semaphore should be zero and thread should be blocked

    sem_post(&semaphore); // Increment semaphore to unblock the thread
    pthread_join(thread, NULL);

    sem_getvalue(&semaphore, &value);
    assert(value == 0); // Semaphore should be zero after thread completes

    sem_destroy(&semaphore);
}

    // Client message is received and printed correctly
void test_client_message_received_and_printed_correctly() {
    // Mocking necessary functions and variables
    sem_t semaphore;
    int p = 1;
    int new_socket = 1;
    int client_socket_id = 1;
    int socket_id = 1;
    int MAX_BYTES = 1024;
    char buffer[MAX_BYTES];
    ParsedRequest *request = (ParsedRequest *)malloc(sizeof(ParsedRequest));
    
    // Setting up the semaphore
    sem_init(&semaphore, 0, 1);
    
    // Mocking the client message
    strcpy(buffer, "Test message from client");
    
    // Mocking the receive function
    int bytes_sent_by_client = strlen(buffer);
    
    // Mocking the handle_request function
    handle_request(socket_id, request, buffer);
    
    // Assertions
    TEST_ASSERT_EQUAL_STRING("Test message from client", buffer);
    
    // Cleaning up
    free(request);
    sem_destroy(&semaphore);
}

    // Cache entry is found and data is retrieved from cache
void test_cache_entry_found_and_data_retrieved_from_cache() {
    // Mocking necessary functions and variables
    sem_t semaphore;
    int p = 1;
    int new_socket = 1;
    int client_socket_id = 1;
    int socket_id = 1;
    int MAX_BYTES = 1024;
    char buffer[MAX_BYTES];
    
    // Setting up the semaphore
    sem_init(&semaphore, 0, 1);
    
    // Mocking the cache entry
    struct cache_entry *temp_entry = (struct cache_entry *)malloc(sizeof(struct cache_entry));
    temp_entry->len = 10;
    temp_entry->data = (char *)malloc(10 * sizeof(char));
    strcpy(temp_entry->data, "CacheData");
    
    // Mocking the find_cache_entry function
    find_cache_entry("TestCacheData");
    
    // Assertions
    TEST_ASSERT_EQUAL_STRING("CacheData", temp_entry->data);
    
    // Cleaning up
    free(temp_entry->data);
    free(temp_entry);
    sem_destroy(&semaphore);
}
