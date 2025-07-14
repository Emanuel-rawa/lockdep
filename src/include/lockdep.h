// ARCHITECTURE OVERVIEW:
//
// 1. LOCK GRAPH: All locks should be tracked as nodes in a directed graph where
//    edges represent ordering dependencies (A → B means A acquired before B).
//
// 2. THREAD TRACKING: Each thread should maintain a stack of currently held
//    locks to detect nested locking patterns and build dependencies.
//
// 3. DEADLOCK DETECTION: The system checks for cycles in the lock graph
//    to detect potential deadlocks. If a cycle is found, the system should
//    identify the lock and prevent the acquisition that would lead to a
//    deadlock.

#ifndef LOCKDEP_H
#define LOCKDEP_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

typedef struct adjacent_locks adjacency_locks_t;

// Types of synchronization
typedef enum sync_type {
    SYNC_MUTEX,
    SYNC_RWLOCK,
    SYNC_SEMAPHORE,
    SYNC_CONDVAR
} sync_type_t;

// Node representing a lock in the lock dependency graph.
typedef struct lock_node {
    const void* lock_addr;       // Address of the lock.
    sync_type_t type;            // Type of synchronization primitive
    adjacency_locks_t* children; // List of adjacent (child) locks.
    struct lock_node* next;      // Next lock node in the list.
} lock_node_t;

// Represents an adjacency (edge) in the lock dependency graph.
typedef struct adjacent_locks {
    lock_node_t* lock;           // Pointer to the adjacent lock node.
    struct adjacent_locks* next; // Next adjacency in the list.
} adjacency_locks_t;

// Represents a lock currently held by a thread.
typedef struct held_lock {
    lock_node_t* lock;      // Pointer to the held lock node.
    struct held_lock* next; // Next held lock in the list.
} held_lock_t;

// Context information for a thread, including held locks.
typedef struct thread_context {
    pthread_t thread_id;         // Thread identifier.
    held_lock_t* held_locks;     // List of locks currently held by the thread.
    struct thread_context* next; // Next thread context in the list.
} thread_context_t;

// Memory arena interface
typedef struct memory_arena {
    void* base_ptr;
    size_t size;
    size_t used;
    struct memory_arena* next;
} memory_arena_t;

void lockdep_init(void);

// Register the acquisition of a lock by the current thread. `lock_addr` is the
// address of the lock being acquired. Returns true if acquisition is allowed,
// false if it would cause a deadlock.
bool lockdep_acquire_lock(const void* lock_addr, sync_type_t type);

// Register the release of a lock by the current thread. `lock_addr` is the
// lock being released.
void lockdep_release_lock(const void* lock_addr);

// Functions for each type of primitive
bool lockdep_acquire_mutex(const void* mutex_addr);
bool lockdep_acquire_rwlock_read(const void* rwlock_addr);
bool lockdep_acquire_rwlock_write(const void* rwlock_addr);
bool lockdep_acquire_semaphore(const void* sem_addr);
bool lockdep_wait_condvar(const void* condvar_addr, const void* mutex_addr);

void lockdep_release_mutex(const void* mutex_addr);
void lockdep_release_rwlock(const void* rwlock_addr);
void lockdep_release_semaphore(const void* sem_addr);
void lockdep_signal_condvar(const void* condvar_addr);

// Memory arena
void* arena_alloc(size_t size);
void arena_reset(void);
void arena_destroy(void);

// For disabling lockdep without recompilation.
extern bool lockdep_enabled;

#endif // LOCKDEP_H

