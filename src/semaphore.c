#include "semaphore.h"

#include <stddef.h>
#include <stdint.h>

#include "mutex.h"

void semaphore_init(struct semaphore* s) {
    init(&s->lock);
    s->value = 0;
    s->head = NULL;
    s->tail = NULL;
}

// increment
void semaphore_signal(struct semaphore* s) {
    lock(&s->lock);
    struct waiter* waiter = s->head;
    if (waiter != NULL) {
        s->head = waiter->next;
        if (!s->head) {
            s->tail = NULL;
        }
    }
    s->value++;
    unlock(&s->lock);
    if (waiter != NULL) {
        unlock(&waiter->m);
    }
}

// decrement
void semaphore_wait(struct semaphore* s) {
    for (;;) {
        lock(&s->lock);
        if (s->value > 0) {
            s->value--;
            unlock(&s->lock);
            return;
        }

        struct waiter new;
        init(&new.m);
        lock(&new.m);
        new.next = NULL;
        if (s->tail == NULL) {
            s->head = &new;
        } else {
            s->tail->next = &new;
        }
        s->tail = &new;

        unlock(&s->lock);
        lock(&new.m);
    }
}
