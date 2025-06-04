# TODO's

## Core

- [ ] Add thread safety protection for global state (lock_registry, dependency_list, thread_contexts)
- [ ] Implement basic lock node management (find/create lock nodes)
- [ ] Implement thread context management (find/create thread contexts)
- [ ] Implement held lock stack operations (push/pop for nested locks)
- [ ] Implement dependency edge management (add/find dependencies)
- [ ] Implement cycle detection algorithm (DFS based? Linux is like this)
- [ ] Implement cleanup in lockdep_cleanup()

## Performance

- [ ] Optimize lock node lookups (consider hash tables vs linear search)
- [ ] Optimize thread context lookups for better performance
- [ ] Consider memory pools for frequent allocations/deallocations
- [ ] Add configuration for max dependency graph size to prevent memory exhaustion

## Interposition

- [ ] Complete pthread function interposition (lock/unlock/trylock)
- [ ] Add support for other synchronization primitives (semaphores, condition variables)
- [ ] Implement recursion detection to avoid lockdep validating itself

## Testing

- [ ] Create more stressful test programs to cover various locking scenarios
- [ ] Add performance benchmarks comparing with/without lockdep

## Docs

- [ ] Document performance overhead and limitations

## Resources

- [lockdep-design.txt - The Linux Kernel Archives](https://www.kernel.org/doc/Documentation/locking/lockdep-design.txt)
- [Writing the Ultimate Locking Check - Oracle Blogs](https://blogs.oracle.com/linux/post/writing-the-ultimate-locking-check)
- [Effective Static Detection of Interrupt-Based Deadlocks in Linux](https://www.usenix.org/conference/usenixsecurity24/presentation/ye)
- [What Lockdep Does and Doesn't - Byungchul Park, LG Electronics](https://www.youtube.com/watch?v=KjmIXN-SQOw)
- [GDB automatic deadlock detector](https://github.com/DamZiobro/gdb-automatic-deadlock-detector)
- [Floyd's Linked List Cycle Finding Algorithm](https://cp-algorithms.com/others/tortoise_and_hare.html)
