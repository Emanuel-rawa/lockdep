#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lockdep.h"

bool lockdep_enabled = true;

static lock_node_t* lock_registry;
static pthread_mutex_t lockdep_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* smalloc(const size_t bytes)
{
    void* ptr = malloc(bytes);
    if (!ptr) {
        fprintf(stderr, "out of memory, aborting\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static lock_node_t* find_or_create_lock(const void* lock_addr)
{
    lock_node_t* lock = lock_registry;

    while (lock) {
        if (lock->lock_addr == lock_addr)
            return lock;
        lock = lock->next;
    }

    lock = smalloc(sizeof(lock_node_t));
    lock->children = NULL;
    lock->lock_addr = lock_addr;
    lock->next = lock_registry;
    return lock_registry = lock;
}

void lockdep_init(void)
{
    const char* env = getenv("LOCKDEP_DISABLE");
    if (env && strcmp(env, "1") == 0) {
        lockdep_enabled = false;
        return;
    }

    fprintf(stderr, "[LOCKDEP] Lockdep initialized\n");
}

bool lockdep_acquire_lock(const void* lock_addr)
{
    printf("[LOCKDEP] Acquiring lock  %p\n", lock_addr);

    pthread_mutex_lock(&lockdep_mutex);

    lock_node_t* lock = find_or_create_lock(lock_addr);

    pthread_mutex_unlock(&lockdep_mutex);

    return false;
}

void lockdep_release_lock(const void* lock_addr)
{
    printf("[LOCKDEP] Releasing lock %p\n", lock_addr);

    pthread_mutex_lock(&lockdep_mutex);


    pthread_mutex_unlock(&lockdep_mutex);
}
