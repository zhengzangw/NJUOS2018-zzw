typedef struct __lock_t {int flag;} lock_t;
void cli() { __asm__ __volatile__ ("cli");}
void sti() { __asm__ __volatile__ ("sti");}

char CompareAndSwap(int *ptr, int old, int nov){
    unsigned char ret;
    __asm__ __volatile__ (
        " lock\n"
        " cmpxchgl %2, %1\n"
        " sete %0\n"
        : "=q" (ret), "=m" (*ptr)
        : "r" (nov), "m" (*ptr), "a"(old)
        : "memory"
    );
    return ret;
}

void init(lock_t *mutex) {
    mutex->flag = 0;
}

void lock(lock_t *mutex) {
    cli();
    while (CompareAndSwap(&mutex->flag, 0, 1)==1);
}

void unlock(lock_t *mutex){
    mutex->flag = 0;
    sti();
}