#include "condvar.h"

#include <stddef.h>

#include "mutex.h"

void cv_init(struct condvar* c) {
    init(&c->lock);
    c->head = NULL;
    c->tail = NULL;
}

void cv_wake_all(struct condvar* c) {
    struct waiter* waiter;

    lock(&c->lock);
    waiter = c->head;
    c->head = NULL;
    c->tail = NULL;
    unlock(&c->lock);

    while (waiter != NULL) {
        struct waiter* next = waiter->next;
        unlock(&waiter->m);
        waiter = next;
    }
}

void cv_wake_one(struct condvar* c) {
    lock(&c->lock);
    struct waiter* waiter = c->head;
    if (waiter != NULL) {
        c->head = waiter->next;
        if (c->head == NULL) {
            c->tail = NULL;
        }
    }
    unlock(&c->lock);
    if (waiter != NULL) {
        unlock(&waiter->m);
    }
}

void cv_wait(struct condvar* c, struct mutex* m) {
    struct waiter waiter;
    init(&waiter.m);
    lock(&waiter.m);
    waiter.next = NULL;

    lock(&c->lock);
    if (c->tail != NULL) {
        c->tail->next = &waiter;
    } else {
        c->head = &waiter;
    }
    c->tail = &waiter;
    unlock(&c->lock);
    unlock(m);

    lock(&waiter.m);
    lock(m);
}
