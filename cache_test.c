
    // Successfully adding a cache entry when the size is within limits
void test_add_cache_entry_within_limits() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was added successfully
    assert(result == 1);
    assert(cache_head != NULL);
    assert(strcmp(cache_head->url, url) == 0);
    assert(strcmp(cache_head->data, data) == 0);
    assert(cache_head->len == size);
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Handling the scenario when the cache size exceeds the maximum allowed size
void test_add_cache_entry_exceeds_max_size() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = MAX_CACHE_ELEMENT_SIZE; // Set size to exceed max element size
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was not added due to exceeding size
    assert(result == 0);
    assert(cache_head == NULL);
    
    // Clean up
    pthread_mutex_destroy(&lock);
}

    // Properly locking and unlocking the mutex during cache entry addition
void test_proper_mutex_lock_unlock() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was added successfully
    assert(result == 1);
    assert(cache_head != NULL);
    assert(strcmp(cache_head->url, url) == 0);
    assert(strcmp(cache_head->data, data) == 0);
    assert(cache_head->len == size);
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Correctly allocating memory for the new cache entry and its data
void test_correct_memory_allocation_for_cache_entry() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was added successfully
    assert(result == 1);
    assert(cache_head != NULL);
    assert(strcmp(cache_head->url, url) == 0);
    assert(strcmp(cache_head->data, data) == 0);
    assert(cache_head->len == size);
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Properly setting the timestamp for the new cache entry
void test_properly_setting_timestamp_for_new_cache_entry() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the timestamp is set properly
    assert(result == 1);
    assert(cache_head != NULL);
    assert(cache_head->timestamp > 0); // Check if timestamp is set to a valid value
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Correctly updating the cache linked list with the new entry
void test_correctly_updating_cache_linked_list() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was added successfully
    assert(result == 1);
    assert(cache_head != NULL);
    assert(strcmp(cache_head->url, url) == 0);
    assert(strcmp(cache_head->data, data) == 0);
    assert(cache_head->len == size);
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Handling the scenario when the cache element size exceeds the maximum allowed element size
void test_handling_cache_element_size_exceeds_maximum() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    // Set the maximum cache element size to a small value for testing
    #define MAX_CACHE_ELEMENT_SIZE 10
    
    // Add a cache entry that exceeds the maximum allowed element size
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was not added due to exceeding the maximum size
    assert(result == 0);
    assert(cache_head == NULL);
    
    // Clean up
    pthread_mutex_destroy(&lock);
}

    // Handling memory allocation failures for the new cache entry or its data
void test_handling_memory_allocation_failures() {
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    // Simulate memory allocation failure for cache entry
    will_return_always(__wrap_malloc, NULL);
    int result = add_cache_entry("http://example.com", "example data", 12);
    
    // Check if the entry was not added due to memory allocation failure
    assert(result == 0);
    assert(cache_head == NULL);
    
    // Clean up
    pthread_mutex_destroy(&lock);
}

    // Properly removing cache entries when the cache is full before adding a new entry
void test_properly_removing_cache_entries_when_full() {
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    // Add entries to fill up the cache
    add_cache_entry("http://example1.com", "data1", strlen("data1"));
    add_cache_entry("http://example2.com", "data2", strlen("data2"));
    add_cache_entry("http://example3.com", "data3", strlen("data3"));
    
    // Add a new entry that exceeds the cache size
    add_cache_entry("http://example4.com", "data4", strlen("data4"));
    
    // Check if the new entry was added successfully after removing old entries
    assert(cache_head != NULL);
    assert(strcmp(cache_head->url, "http://example4.com") == 0);
    assert(strcmp(cache_head->data, "data4") == 0);
    assert(cache_head->len == strlen("data4"));
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Ensuring the mutex is unlocked even if an error occurs during the process
void test_mutex_unlock_on_error() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    // Simulate error by setting element_size greater than MAX_CACHE_ELEMENT_SIZE
    int result = add_cache_entry(url, data, MAX_CACHE_ELEMENT_SIZE);
    
    // Check if the mutex is unlocked even if an error occurs
    assert(result == 0);
    assert(cache_head == NULL);
    
    // Clean up
    pthread_mutex_destroy(&lock);
}
// Ensure that the mutex is unlocked even if an error occurs during the process

    // Verifying the integrity of the cache linked list after adding a new entry
void test_add_cache_entry_integrity() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was added successfully
    assert(result == 1);
    assert(cache_head != NULL);
    assert(strcmp(cache_head->url, url) == 0);
    assert(strcmp(cache_head->data, data) == 0);
    assert(cache_head->len == size);
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Ensuring the cache size is updated correctly after adding a new entry
void test_cache_size_updated_correctly_after_adding_entry() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was added successfully
    assert(result == 1);
    assert(cache_head != NULL);
    assert(strcmp(cache_head->url, url) == 0);
    assert(strcmp(cache_head->data, data) == 0);
    assert(cache_head->len == size);
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Validating the timestamp accuracy for the new cache entry
void test_validate_timestamp_accuracy() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the timestamp is set to the current time
    time_t current_time = time(NULL);
    assert(result == 1);
    assert(cache_head != NULL);
    assert(cache_head->timestamp <= current_time);
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

    // Checking the behavior when the URL or data is an empty string
void test_add_cache_entry_with_empty_url() {
    char url[] = "";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was not added due to empty URL
    assert(result == 0);
    assert(cache_head == NULL);
    
    // Clean up
    pthread_mutex_destroy(&lock);
}

    // Ensuring thread safety when multiple threads attempt to add cache entries simultaneously
void test_add_cache_entry_thread_safety() {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    // Initialize the cache and mutex
    cache_head = NULL;
    cache_size = 0;
    pthread_mutex_init(&lock, NULL);
    
    // Create multiple threads to add cache entries simultaneously
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, add_cache_entry_thread, (void *)i);
    }
    
    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Check if the entries were added successfully
    assert(cache_size == NUM_THREADS * (size + 1 + strlen(url) + sizeof(cache_entry)));
    
    // Clean up
    pthread_mutex_destroy(&lock);
    free(cache_head->data);
    free(cache_head->url);
    free(cache_head);
}

void *add_cache_entry_thread(void *thread_id) {
    char url[] = "http://example.com";
    char data[] = "example data";
    int size = strlen(data);
    
    int result = add_cache_entry(url, data, size);
    
    // Check if the entry was added successfully
    assert(result == 1);
    
    pthread_exit(NULL);
}
