#include <common.h>
#include <kmt.h>

// ============= Thread   ============

task_t *tasks[MAXTASK];
int h_tasks;

void init(){

}

int create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    //TODO: ADD ERROR DETECT
    task->name = name;
    task->exists = 1;
    task->run = 0;
    task->stack = pmm->alloc(STACKSIZE);
    task->kcontext((_Area){task->stack, task->stack+STACKSIZE}, entry, arg);

    int i;
    for (i=h_tasks;!empty(tasks[i]);i=(i+1)%MAXTASK);
    h_tasks = i;
    tasks[h_tasks] = task;

    return 0;
}
void teardown(task_t *task){
    pmm->free(task->stack);
    task->exists = 0;
}


// ============= Spinlock ============

int cpuncli[64], intena[64];

int readflags(){
    uint eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

static void pushcli(void){
    int eflags;
    eflags = readflags();
    _intr_write(0);
    if (cpuncli[_cpu()] == 0) intena[_cpu()] = eflags & FL_IF;
    cpuncli[_cpu()]+=1;
}

static void popcli(void){
    assertIF0();
    if (--cpuncli[_cpu()]<0) Panic("popcli");
    if (cpuncli[_cpu()]==0 && intena[_cpu()]) _intr_write(1);
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
