#ifndef CONDVAR_H
#define CONDVAR_H

#include "mutex.h"

struct waiter {
    struct mutex m;
    struct waiter* next;
};

struct condvar {
    struct mutex lock;
    struct waiter* head;
    struct waiter* tail;
};

void cv_init(struct condvar* c);
void cv_wake_one(struct condvar* c);
void cv_wake_all(struct condvar* c);
void cv_wait(struct condvar* c, struct mutex* m);

#endif  // !CONDVAR_H
