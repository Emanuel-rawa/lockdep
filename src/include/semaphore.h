#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

#include "mutex.h"

struct waiter {
    struct mutex m;
    struct waiter* next;
};

struct semaphore {
    struct mutex lock;
    uint64_t value;
    struct waiter* head;
    struct waiter* tail;
};

void semaphore_init(struct semaphore* s);
void semaphore_signal(struct semaphore* s);
void semaphore_wait(struct semaphore* s);

#endif  // !SEMAPHORE_H
