#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>

struct mutex {
    _Atomic uint32_t lock;
};

void init(struct mutex* m);
void lock(struct mutex* m);
void unlock(struct mutex* m);

#endif  // !MUTEX_H
