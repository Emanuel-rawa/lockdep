#include "mutex.h"

#include <bits/time.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

void init(struct mutex* m) { m->lock = 0; }

void lock(struct mutex* m) {
    uint32_t v = 0;
    if (atomic_compare_exchange_strong_explicit(
            &m->lock, &v, 1, memory_order_acquire, memory_order_relaxed)) {
        return;
    }
    do {
        if (v == 2 ||
            atomic_compare_exchange_strong_explicit(
                &m->lock, &v, 2, memory_order_relaxed, memory_order_relaxed)) {
            syscall(SYS_futex, &m->lock, FUTEX_WAIT, 2);
        }
        v = 0;
    } while (!atomic_compare_exchange_strong_explicit(
        &m->lock, &v, 2, memory_order_acquire, memory_order_relaxed));
}

void unlock(struct mutex* m) {
    uint32_t v = atomic_fetch_sub_explicit(&m->lock, 1, memory_order_release);
    if (v != 1) {
        atomic_store_explicit(&m->lock, 0, memory_order_relaxed);
        syscall(SYS_futex, &m->lock, FUTEX_WAKE, 1);
    }
}
