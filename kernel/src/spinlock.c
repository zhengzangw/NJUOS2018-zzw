#include <common.h>
#include <debug.h>

#define cli() asm volatile ("cli" ::: "memory");
#define sti() asm volatile ("sti" ::: "memory");

static inline int readflags(){
    uint eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}
#define FL_IF 0x00000200
#define checkIF() if (readflags()&FL_IF) panic("interruptible");

uint cpuncli[64];

void pushcli(void){
    cli();
    cpuncli[_cpu()]+=1;
}

void popcli(void){
    checkIF();
    if (--cpuncli[_cpu()]<0) panic("popcli");
    if (cpuncli[_cpu()]==0) sti();
}

int holding(struct spinlock *lock){
    int r;
    pushcli();
    r = lock->locked && lock->cpu==_cpu();
    popcli();
    return r;
}

void initlock(struct spinlock *lk){
    lk->cpu = 0;
    lk->locked = 0;
}

void acquire(struct spinlock *lk){
    pushcli();
    if (holding(lk)) panic("acquire");

    while (_atomic_xchg(&lk-locked, 1)!=0);

    __sync_synchronize();
    lk->cpu = _cpu();
}

void release(struct spinlock *lk) {
    if (!holding(lk)) panic("release");
    lk->cpu = 0;

    __sync_synchronize();

    asm volatile ("movl $0, %0" : "+m" (lk-locked) :);
    popcli();
}
