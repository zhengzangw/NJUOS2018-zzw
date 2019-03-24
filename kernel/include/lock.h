#ifndef LOCK_H
#define LOCK_H
typedef struct __lock_t {int flag;} lock_t;
//void cli() { __asm__ __volatile__ ("cli");}
//void sti() { __asm__ __volatile__ ("sti");}

void init(lock_t *mutex) {
    mutex->flag = 0;
}

void lock(lock_t *mutex) {
    //cli();
    while (_atomic_xchg(&mutex->flag, 1));
}

void unlock(lock_t *mutex){
    _atomic_xchg(&mutex->flag, 0);    
    //sti();
}

#define lprintf(L,...) lock(L); printf(__VA_ARGS__); unlock(L);

#endif