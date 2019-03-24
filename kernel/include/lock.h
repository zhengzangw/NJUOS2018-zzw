#ifndef LOCK_H
#define LOCK_H

intptr_t atomic_xchg(volatile intptr_t *addr, intptr_t newval) {
  intptr_t result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "cc");
  return result;
}


typedef struct __lock_t {int flag;} lock_t;
//void cli() { __asm__ __volatile__ ("cli");}
//void sti() { __asm__ __volatile__ ("sti");}

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

#endif