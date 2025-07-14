#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#include "../include/lockdep.h"

static int (*real_pthread_mutex_lock)(pthread_mutex_t*) = NULL;
static int (*real_pthread_mutex_unlock)(pthread_mutex_t*) = NULL;
static int (*real_pthread_mutex_trylock)(pthread_mutex_t*) = NULL;

static int (*real_pthread_rwlock_rdlock)(pthread_rwlock_t*) = NULL;
static int (*real_pthread_rwlock_wrlock)(pthread_rwlock_t*) = NULL;
static int (*real_pthread_rwlock_unlock)(pthread_rwlock_t*) = NULL;
static int (*real_pthread_rwlock_tryrdlock)(pthread_rwlock_t*) = NULL;
static int (*real_pthread_rwlock_trywrlock)(pthread_rwlock_t*) = NULL;

static int (*real_sem_wait)(sem_t*) = NULL;
static int (*real_sem_trywait)(sem_t*) = NULL;
static int (*real_sem_post)(sem_t*) = NULL;

static int (*real_pthread_cond_wait)(pthread_cond_t*, pthread_mutex_t*) = NULL;
static int (*real_pthread_cond_timedwait)(pthread_cond_t*, pthread_mutex_t*, const struct timespec*) = NULL;
static int (*real_pthread_cond_signal)(pthread_cond_t*) = NULL;
static int (*real_pthread_cond_broadcast)(pthread_cond_t*) = NULL;

/// This interposes the real pthread functions to add lockdep validation
static void init_real_functions(void)
{
    if (!real_pthread_mutex_lock) {
        real_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    }
    if (!real_pthread_mutex_unlock) {
        real_pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
    }
    if (!real_pthread_mutex_trylock) {
        real_pthread_mutex_trylock = dlsym(RTLD_NEXT, "pthread_mutex_trylock");
    }

    // RWLock functions
    if (!real_pthread_rwlock_rdlock) {
        real_pthread_rwlock_rdlock = dlsym(RTLD_NEXT, "pthread_rwlock_rdlock");
    }
    if (!real_pthread_rwlock_wrlock) {
        real_pthread_rwlock_wrlock = dlsym(RTLD_NEXT, "pthread_rwlock_wrlock");
    }
    if (!real_pthread_rwlock_unlock) {
        real_pthread_rwlock_unlock = dlsym(RTLD_NEXT, "pthread_rwlock_unlock");
    }
    if (!real_pthread_rwlock_tryrdlock) {
        real_pthread_rwlock_tryrdlock = dlsym(RTLD_NEXT, "pthread_rwlock_tryrdlock");
    }
    if (!real_pthread_rwlock_trywrlock) {
        real_pthread_rwlock_trywrlock = dlsym(RTLD_NEXT, "pthread_rwlock_trywrlock");
    }

    // Semaphore functions
    if (!real_sem_wait) {
        real_sem_wait = dlsym(RTLD_NEXT, "sem_wait");
    }
    if (!real_sem_trywait) {
        real_sem_trywait = dlsym(RTLD_NEXT, "sem_trywait");
    }
    if (!real_sem_post) {
        real_sem_post = dlsym(RTLD_NEXT, "sem_post");
    }

    // Condition variable functions
    if (!real_pthread_cond_wait) {
        real_pthread_cond_wait = dlsym(RTLD_NEXT, "pthread_cond_wait");
    }
    if (!real_pthread_cond_timedwait) {
        real_pthread_cond_timedwait = dlsym(RTLD_NEXT, "pthread_cond_timedwait");
    }
    if (!real_pthread_cond_signal) {
        real_pthread_cond_signal = dlsym(RTLD_NEXT, "pthread_cond_signal");
    }
    if (!real_pthread_cond_broadcast) {
        real_pthread_cond_broadcast = dlsym(RTLD_NEXT, "pthread_cond_broadcast");
    }
}

__attribute__((constructor)) static void lockdep_constructor(void)
{
    lockdep_init();
    init_real_functions();
}

/// The lockdep uses a mutex to protect its internal state, so we use this to
/// avoid recursing lockdep validation across itself.
static __thread bool in_interpose = false;

// ==================== MUTEX FUNCTIONS ====================

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    init_real_functions();

    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_mutex(mutex)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED\n");
            in_interpose = false;
            return EDEADLK;
        }
        in_interpose = false;
    }

    int result = real_pthread_mutex_lock(mutex);
    return result;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    init_real_functions();

    int result = real_pthread_mutex_unlock(mutex);
    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        lockdep_release_mutex(mutex);
        in_interpose = false;
    }

    return result;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    init_real_functions();

    int result = real_pthread_mutex_trylock(mutex);
    if (result == 0 && lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_mutex(mutex)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on trylock - unlocking and failing\n");
            real_pthread_mutex_unlock(mutex);
            in_interpose = false;
            return EBUSY;
        }
        in_interpose = false;
    }

    return result;
}

// ==================== RWLOCK FUNCTIONS ====================

int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock)
{
    init_real_functions();

    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_rwlock_read(rwlock)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on rwlock_rdlock\n");
            in_interpose = false;
            return EDEADLK;
        }
        in_interpose = false;
    }

    int result = real_pthread_rwlock_rdlock(rwlock);
    return result;
}

int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock)
{
    init_real_functions();

    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_rwlock_write(rwlock)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on rwlock_wrlock\n");
            in_interpose = false;
            return EDEADLK;
        }
        in_interpose = false;
    }

    int result = real_pthread_rwlock_wrlock(rwlock);
    return result;
}

int pthread_rwlock_unlock(pthread_rwlock_t* rwlock)
{
    init_real_functions();

    int result = real_pthread_rwlock_unlock(rwlock);
    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        lockdep_release_rwlock(rwlock);
        in_interpose = false;
    }

    return result;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock)
{
    init_real_functions();

    int result = real_pthread_rwlock_tryrdlock(rwlock);
    if (result == 0 && lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_rwlock_read(rwlock)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on rwlock_tryrdlock - unlocking and failing\n");
            real_pthread_rwlock_unlock(rwlock);
            in_interpose = false;
            return EBUSY;
        }
        in_interpose = false;
    }

    return result;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t* rwlock)
{
    init_real_functions();

    int result = real_pthread_rwlock_trywrlock(rwlock);
    if (result == 0 && lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_rwlock_write(rwlock)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on rwlock_trywrlock - unlocking and failing\n");
            real_pthread_rwlock_unlock(rwlock);
            in_interpose = false;
            return EBUSY;
        }
        in_interpose = false;
    }

    return result;
}

// ==================== SEMAPHORE FUNCTIONS ====================

int sem_wait(sem_t* sem)
{
    init_real_functions();

    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_semaphore(sem)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on sem_wait\n");
            in_interpose = false;
            return EDEADLK;
        }
        in_interpose = false;
    }

    int result = real_sem_wait(sem);
    return result;
}

int sem_trywait(sem_t* sem)
{
    init_real_functions();

    int result = real_sem_trywait(sem);
    if (result == 0 && lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_acquire_semaphore(sem)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on sem_trywait\n");
            in_interpose = false;
            return EAGAIN;
        }
        in_interpose = false;
    }

    return result;
}

int sem_post(sem_t* sem)
{
    init_real_functions();

    int result = real_sem_post(sem);
    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        lockdep_release_semaphore(sem);
        in_interpose = false;
    }

    return result;
}

// ==================== CONDITION VARIABLE FUNCTIONS ====================

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    init_real_functions();

    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_wait_condvar(cond, mutex)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on cond_wait\n");
            in_interpose = false;
            return EDEADLK;
        }
        in_interpose = false;
    }

    int result = real_pthread_cond_wait(cond, mutex);
    return result;
}

int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* abstime)
{
    init_real_functions();

    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        if (!lockdep_wait_condvar(cond, mutex)) {
            fprintf(stderr, "[LOCKDEP] DEADLOCK DETECTED on cond_timedwait\n");
            in_interpose = false;
            return EDEADLK;
        }
        in_interpose = false;
    }

    int result = real_pthread_cond_timedwait(cond, mutex, abstime);
    return result;
}

int pthread_cond_signal(pthread_cond_t* cond)
{
    init_real_functions();

    int result = real_pthread_cond_signal(cond);
    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        lockdep_signal_condvar(cond);
        in_interpose = false;
    }

    return result;
}

int pthread_cond_broadcast(pthread_cond_t* cond)
{
    init_real_functions();

    int result = real_pthread_cond_broadcast(cond);
    if (lockdep_enabled && !in_interpose) {
        in_interpose = true;
        lockdep_signal_condvar(cond);
        in_interpose = false;
    }

    return result;
}
