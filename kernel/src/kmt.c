#include <kmt.h>

// ============= Thread   ============

void init(){
}
int create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    return 0;
}
void teardown(task_t *task){
}


// ============= Spinlock ============

int cpuncli[64];

static void pushcli(void){
    cli();
    cpuncli[_cpu()]+=1;
}

static void popcli(void){
    assertIF1();
    if (--cpuncli[_cpu()]<0) Panic("popcli");
    if (cpuncli[_cpu()]==0) sti();
}

static int holding(spinlock_t *lock){
    int r;
    pushcli();
    r = lock->locked && lock->cpu==_cpu();
    popcli();
    return r;
}

void spin_init(spinlock_t *lk, const char *name){
    lk->cpu = 0;
    lk->locked = 0;
    lk->name = name;
}

void spin_lock(spinlock_t *lk){
    pushcli();
    Assert(!holding(lk), "spin_lock");

    while (_atomic_xchg(&lk->locked, 1)!=0);

    __sync_synchronize();
    lk->cpu = _cpu();
}

void spin_unlock(spinlock_t *lk) {
    Assert(holding(lk), "spin_unlock");
    lk->cpu = 0;

    __sync_synchronize();

    asm volatile ("movl $0, %0" : "+m" (lk->locked) :);
    popcli();
}

// ============= Semaphore ==============

void sem_init(sem_t *sem, const char *name, int value){

}
void sem_wait(sem_t *sem){

}
void sem_signal(sem_t *sem){

}

MODULE_DEF(kmt) {
    .init = init,
    .create = create,
    .teardown = teardown,
    .spin_init = spin_init,
    .spin_lock = spin_lock,
    .spin_unlock = spin_unlock,
    .sem_init = sem_init,
    .sem_wait = sem_wait,
    .sem_signal = sem_signal,
};
