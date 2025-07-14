#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../include/lockdep.h"

bool lockdep_enabled = true;

static lock_node_t* lock_registry;
static thread_context_t* thread_registry;
static pthread_mutex_t lockdep_mutex = PTHREAD_MUTEX_INITIALIZER;

// ==================== MEMORY ARENA ====================

#define ARENA_SIZE (1024 * 1024) // 1MB por arena
#define ARENA_ALIGNMENT 8

static memory_arena_t* arena_list = NULL;
static pthread_mutex_t arena_mutex = PTHREAD_MUTEX_INITIALIZER;

static size_t align_size(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

static memory_arena_t* create_arena(size_t min_size)
{
    size_t arena_size = (min_size > ARENA_SIZE) ? align_size(min_size, getpagesize()) : ARENA_SIZE;

    void* memory = mmap(NULL, arena_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        fprintf(stderr, "[LOCKDEP] Failed to allocate arena memory\n");
        return NULL;
    }

    memory_arena_t* arena = (memory_arena_t*)memory;
    arena->base_ptr = memory;
    arena->size = arena_size;
    arena->used = align_size(sizeof(memory_arena_t), ARENA_ALIGNMENT);
    arena->next = NULL;

    return arena;
}

void* arena_alloc(size_t size)
{
    if (size == 0) return NULL;

    size = align_size(size, ARENA_ALIGNMENT);

    pthread_mutex_lock(&arena_mutex);

    memory_arena_t* arena = arena_list;
    while (arena) {
        if (arena->used + size <= arena->size) {
            void* ptr = (char*)arena->base_ptr + arena->used;
            arena->used += size;
            pthread_mutex_unlock(&arena_mutex);
            return ptr;
        }
        arena = arena->next;
    }

    arena = create_arena(size);
    if (!arena) {
        pthread_mutex_unlock(&arena_mutex);
        return NULL;
    }

    arena->next = arena_list;
    arena_list = arena;

    void* ptr = (char*)arena->base_ptr + arena->used;
    arena->used += size;

    pthread_mutex_unlock(&arena_mutex);
    return ptr;
}

void arena_reset(void)
{
    pthread_mutex_lock(&arena_mutex);

    memory_arena_t* arena = arena_list;
    while (arena) {
        arena->used = align_size(sizeof(memory_arena_t), ARENA_ALIGNMENT);
        arena = arena->next;
    }

    pthread_mutex_unlock(&arena_mutex);
}

void arena_destroy(void)
{
    pthread_mutex_lock(&arena_mutex);

    memory_arena_t* arena = arena_list;
    while (arena) {
        memory_arena_t* next = arena->next;
        munmap(arena->base_ptr, arena->size);
        arena = next;
    }

    arena_list = NULL;
    pthread_mutex_unlock(&arena_mutex);
}

// ==================== ALLOCATION FUNCTIONS ====================

static void* smalloc(const size_t bytes)
{
    void* ptr = arena_alloc(bytes);
    if (!ptr) {
        fprintf(stderr, "[LOCKDEP] Arena allocation failed, falling back to malloc\n");
        ptr = malloc(bytes);
        if (!ptr) {
            fprintf(stderr, "[LOCKDEP] Out of memory, aborting\n");
            exit(EXIT_FAILURE);
        }
    }
    return ptr;
}

// ==================== LOCK MANIPULATION FUNCTIONS ====================

static const char* sync_type_to_string(sync_type_t type)
{
    switch (type) {
    case SYNC_MUTEX:
        return "MUTEX";
    case SYNC_RWLOCK:
        return "RWLOCK";
    case SYNC_SEMAPHORE:
        return "SEMAPHORE";
    case SYNC_CONDVAR:
        return "CONDVAR";
    default:
        return "UNKNOWN";
    }
}

static lock_node_t* find_or_create_lock(const void* lock_addr, sync_type_t type)
{
    lock_node_t* lock = lock_registry;

    while (lock) {
        if (lock->lock_addr == lock_addr) {
            if (lock->type != type) {
                lock->type = type;
            }
            return lock;
        }
        lock = lock->next;
    }

    lock = smalloc(sizeof(lock_node_t));
    lock->children = NULL;
    lock->lock_addr = lock_addr;
    lock->type = type;
    lock->next = lock_registry;
    return lock_registry = lock;
}

static thread_context_t* find_thread_context(const pthread_t thread_id)
{
    thread_context_t* ctx = thread_registry;
    while (ctx) {
        if (ctx->thread_id == thread_id) return ctx;
        ctx = ctx->next;
    }
    return NULL;
}

static void add_dependency(lock_node_t* parent, lock_node_t* child)
{
    adjacency_locks_t* adj = parent->children;
    while (adj) {
        if (adj->lock == child) return;
        adj = adj->next;
    }

    adj = smalloc(sizeof(adjacency_locks_t));
    adj->lock = child;
    adj->next = parent->children;
    parent->children = adj;
}

static bool has_cycle_dfs(lock_node_t* node, lock_node_t* target, lock_node_t** visited, int* visit_count)
{
    for (int i = 0; i < *visit_count; i++) {
        if (visited[i] == node) return false;
    }

    visited[(*visit_count)++] = node;

    if (node == target) return true;

    adjacency_locks_t* adj = node->children;
    while (adj) {
        if (has_cycle_dfs(adj->lock, target, visited, visit_count)) {
            return true;
        }
        adj = adj->next;
    }

    return false;
}

static bool would_create_cycle(lock_node_t* from, lock_node_t* to)
{
    lock_node_t** visited = smalloc(sizeof(lock_node_t*) * 1000); // Limite arbitrário
    int visit_count = 0;

    bool cycle_found = has_cycle_dfs(to, from, visited, &visit_count);

    return cycle_found;
}

static thread_context_t* add_lock_to_thread_context(thread_context_t* ctx, lock_node_t* lock)
{
    if (ctx) {
        held_lock_t* new_held = smalloc(sizeof(held_lock_t));
        new_held->lock = lock;
        new_held->next = ctx->held_locks;
        ctx->held_locks = new_held;
        return ctx;
    }

    ctx = smalloc(sizeof(thread_context_t));
    ctx->held_locks = smalloc(sizeof(held_lock_t));
    ctx->thread_id = pthread_self();
    ctx->held_locks->lock = lock;
    ctx->held_locks->next = NULL;
    ctx->next = thread_registry;
    thread_registry = ctx;
    return ctx;
}

static thread_context_t* release_lock_from_thread_context(thread_context_t* ctx, const void* lock_addr)
{
    if (!ctx) return NULL;

    held_lock_t** held = &ctx->held_locks;
    while (*held) {
        if ((*held)->lock->lock_addr == lock_addr) {
            *held = (*held)->next;
            // free memory with arena
            break;
        }
        held = &(*held)->next;
    }
    return ctx;
}

// ==================== PUBLIC FUNCTIONS ====================

void lockdep_init(void)
{
    const char* env = getenv("LOCKDEP_DISABLE");
    if (env && strcmp(env, "1") == 0) {
        lockdep_enabled = false;
        return;
    }

    fprintf(stderr, "[LOCKDEP] Lockdep initialized with extended synchronization support\n");
}

bool lockdep_acquire_lock(const void* lock_addr, sync_type_t type)
{
    printf("[LOCKDEP] Acquiring %s lock %p\n", sync_type_to_string(type), lock_addr);

    pthread_mutex_lock(&lockdep_mutex);

    lock_node_t* lock = find_or_create_lock(lock_addr, type);
    thread_context_t* ctx = find_thread_context(pthread_self());

    // Verifica dependências com locks já mantidos
    if (ctx && ctx->held_locks) {
        held_lock_t* held = ctx->held_locks;
        while (held) {
            // Adiciona dependência: held_lock -> new_lock
            add_dependency(held->lock, lock);

            // Verifica se criaria um ciclo
            if (would_create_cycle(held->lock, lock)) {
                printf("[LOCKDEP] Cycle detected between %s %p and %s %p\n", sync_type_to_string(held->lock->type),
                       held->lock->lock_addr, sync_type_to_string(lock->type), lock->lock_addr);
                pthread_mutex_unlock(&lockdep_mutex);
                return false;
            }

            held = held->next;
        }
    }

    ctx = add_lock_to_thread_context(ctx, lock);

    // Debug: mostra locks atualmente mantidos
    held_lock_t* held = ctx->held_locks;
    printf("[LOCKDEP] Thread %lu currently holds locks:\n", ctx->thread_id);
    while (held) {
        printf("[LOCKDEP] - %s %p\n", sync_type_to_string(held->lock->type), held->lock->lock_addr);
        held = held->next;
    }

    pthread_mutex_unlock(&lockdep_mutex);
    return true;
}

void lockdep_release_lock(const void* lock_addr)
{
    printf("[LOCKDEP] Releasing lock %p\n", lock_addr);

    pthread_mutex_lock(&lockdep_mutex);

    thread_context_t* ctx = find_thread_context(pthread_self());
    if (ctx) {
        ctx = release_lock_from_thread_context(ctx, lock_addr);

        // Debug: mostra locks atualmente mantidos
        held_lock_t* held = ctx->held_locks;
        printf("[LOCKDEP] Thread %lu currently holds locks:\n", ctx->thread_id);
        while (held) {
            printf("[LOCKDEP] - %s %p\n", sync_type_to_string(held->lock->type), held->lock->lock_addr);
            held = held->next;
        }
    }

    pthread_mutex_unlock(&lockdep_mutex);
}

// ==================== FUNCTIONS FOR EACH TYPE ====================

bool lockdep_acquire_mutex(const void* mutex_addr)
{
    return lockdep_acquire_lock(mutex_addr, SYNC_MUTEX);
}

bool lockdep_acquire_rwlock_read(const void* rwlock_addr)
{
    return lockdep_acquire_lock(rwlock_addr, SYNC_RWLOCK);
}

bool lockdep_acquire_rwlock_write(const void* rwlock_addr)
{
    return lockdep_acquire_lock(rwlock_addr, SYNC_RWLOCK);
}

bool lockdep_acquire_semaphore(const void* sem_addr)
{
    return lockdep_acquire_lock(sem_addr, SYNC_SEMAPHORE);
}

bool lockdep_wait_condvar(const void* condvar_addr, const void* mutex_addr)
{

    printf("[LOCKDEP] Waiting on condvar %p with mutex %p\n", condvar_addr, mutex_addr);

    pthread_mutex_lock(&lockdep_mutex);

    lock_node_t* condvar_lock = find_or_create_lock(condvar_addr, SYNC_CONDVAR);
    thread_context_t* ctx = find_thread_context(pthread_self());

    if (ctx && ctx->held_locks) {
        held_lock_t* held = ctx->held_locks;
        while (held) {
            if (held->lock->lock_addr != mutex_addr) {
                add_dependency(held->lock, condvar_lock);

                if (would_create_cycle(held->lock, condvar_lock)) {
                    printf("[LOCKDEP] Cycle detected in condvar wait\n");
                    pthread_mutex_unlock(&lockdep_mutex);
                    return false;
                }
            }
            held = held->next;
        }
    }

    if (ctx) {
        release_lock_from_thread_context(ctx, mutex_addr);
    }

    pthread_mutex_unlock(&lockdep_mutex);
    return true;
}

void lockdep_release_mutex(const void* mutex_addr)
{
    lockdep_release_lock(mutex_addr);
}

void lockdep_release_rwlock(const void* rwlock_addr)
{
    lockdep_release_lock(rwlock_addr);
}

void lockdep_release_semaphore(const void* sem_addr)
{
    lockdep_release_lock(sem_addr);
}

void lockdep_signal_condvar(const void* condvar_addr)
{
    printf("[LOCKDEP] Signaling condvar %p\n", condvar_addr);
}
