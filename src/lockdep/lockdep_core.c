#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/lockdep.h"

bool lockdep_enabled = true;

void lockdep_init(void) {
    const char* env = getenv("LOCKDEP_DISABLE");
    if (env && strcmp(env, "1") == 0) {
        lockdep_enabled = false;
        return;
    }

    fprintf(stderr, "[LOCKDEP] Lockdep initialized\n");
}

void lockdep_cleanup(void) {
    // TODO: Implement if needed
    return;
}

bool lockdep_acquire_lock(void* lock_addr) {
    printf("[LOCKDEP] Acquiring lock at %p\n", lock_addr);
    return true;
}

void lockdep_release_lock(void* lock_addr) {
    printf("[LOCKDEP] Releasing lock at %p\n", lock_addr);
}
