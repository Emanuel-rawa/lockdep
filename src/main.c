#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "condvar.h"
#include "mutex.h"

#define SIZE 10
volatile int data[SIZE];
volatile size_t ins = 0;
volatile size_t rem = 0;

struct mutex m;
struct condvar cv;

void* producer(void* arg) {
    int v;
    for (v = 1;; v++) {
        while (((ins + 1) % SIZE) == rem) {
            cv_wait(&cv, &m);
        }
        lock(&m);
        printf("%zu: Producing %d\n", (size_t)arg, v);
        data[ins] = v;
        ins = (ins + 1) % SIZE;
        unlock(&m);
        cv_wake_all(&cv);
        sleep(1);
    }
    return NULL;
}

void* consumer(void* arg) {
    for (;;) {
        lock(&m);
        while (ins == rem) {
            cv_wait(&cv, &m);
        }
        printf("%zu: Consuming %d\n", (size_t)arg, data[rem]);
        rem = (rem + 1) % SIZE;
        unlock(&m);
        cv_wake_all(&cv);
    }
    return NULL;
}

int main() {
    pthread_t th1;
    pthread_t th2;
    pthread_t th3;

    init(&m);
    cv_init(&cv);

    pthread_create(&th2, NULL, producer, (void*)0);
    pthread_create(&th1, NULL, consumer, (void*)1);
    pthread_create(&th3, NULL, consumer, (void*)2);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    pthread_join(th3, NULL);

    return 0;
}
