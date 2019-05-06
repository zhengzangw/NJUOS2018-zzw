#include <common.h>
#include <kmt.h>

// ============= Thread   ============

task_t *tasks[MAXTASK];
int h_tasks;
task_t *cputask[MAXCPU];
spinlock_t lock_kmt;

_Context *kmt_context_save(_Event ev, _Context* context){
    if (cputask[_cpu()]!=NULL) {
        if (cputask[_cpu()]->exists!=0){
            cputask[_cpu()]->context = *context;
            Assert(cputask[_cpu()]->run==1, "running threads run=0");
            cputask[_cpu()]->run = 0;
        } else {
            cputask[_cpu()] = NULL;
        }
    }
    return NULL;
}

_Context *kmt_context_switch(_Event ev, _Context * context){
    int cur = cputask[_cpu()]==NULL?0:cputask[_cpu()]->id;
    while (1){
      kmt->spin_lock(&lock_kmt);
      for (int i=0;i<MAXTASK;++i){
        int next = (cur+i+1)%MAXTASK;
        Assert(empty(tasks[next])||task[next]->id==next, "id!=index");
        if (!empty(tasks[next])&&(tasks[next]->run==0||(tasks[next]->id==cur&&cputask[_cpu()]!=NULL))){
            cputask[_cpu()] = tasks[next];
            Logcontext(tasks[next]);
            tasks[next]->run = 1;
            kmt->spin_unlock(&lock_kmt);

            Assert(cputask[_cpu()]->exists==1, "threads prepared to run exists=0");
            return &cputask[_cpu()]->context;
        }
      }
    }
    SHOULD_NOT_REACH_HERE();
    return NULL;
}

void init(){
    kmt->spin_init(&lock_kmt, "kmt");
    os->on_irq(INT_MIN, _EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, _EVENT_NULL, kmt_context_switch);
    for (int i=0;i<MAXTASK;++i){
        tasks[i] = NULL;
    }
    for (int i=0;i<MAXCPU;++i){
        cputask[i] = NULL;
    }
}

int create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    task->name = name;
    task->exists = 1;
    task->run = 0;
    task->stack = pmm->alloc(STACKSIZE);
    if (task->stack==NULL) {
        return 1;
    }
    task->context = *_kcontext((_Area){task->stack, task->stack+STACKSIZE}, entry, arg);

    int i,cnt=0;
    kmt->spin_lock(&lock_kmt);
    for (i=h_tasks;cnt<MAXTASK&&!empty(tasks[i]);i=(i+1)%MAXTASK,cnt++);
    if (cnt==MAXTASK){
        kmt->spin_unlock(&lock_kmt);
        pmm->free(task->stack);
        warning("Create Failed: Task amount overflows\n");
        return 1;
    } else {
        tasks[h_tasks = i] = task;
        task->id = h_tasks;
        kmt->spin_unlock(&lock_kmt);
        return 0;
    }
    SHOULD_NOT_REACH_HERE();
}
void teardown(task_t *task){
    pmm->free(task->stack);
    kmt->spin_lock(&lock_kmt);
    task->exists = 0;
    kmt->spin_unlock(&lock_kmt);
}


// ============= Spinlock ============

int cpuncli[MAXCPU], intena[MAXCPU];

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
    Assert(!holding(lk), "locking a locked lock %s", lk->name);

    while (_atomic_xchg(&lk->locked, 1)!=0){
    //    Loglock(lk);
    //  _putc("12345678"[_cpu()]);
    };

    __sync_synchronize();
    lk->cpu = _cpu();
}

void spin_unlock(spinlock_t *lk) {
    Assert(holding(lk), "release an unlocked lock %s", lk->name);
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
