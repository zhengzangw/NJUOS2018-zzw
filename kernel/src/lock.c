#include <lock.h>
#include <common.h>

static intptr_t atomic_xchg(volatile intptr_t *addr, intptr_t newval) {
  intptr_t result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "cc");
  return result;
}

void init(lock_t *mutex) {
    mutex->flag = 0;
}

void lock(lock_t *mutex) {
    //cli();
    while (atomic_xchg(&mutex->flag, 1));
}

void unlock(lock_t *mutex){
    atomic_xchg(&mutex->flag, 0);    
    //sti();
}