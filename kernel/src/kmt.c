#include <common.h>
#include <kmt.h>

// ============= Thread   ============

task_t *tasks[MAXTASK];
int cnt_tasks; //total created tasks
task_t *cputask[MAXCPU]; //task running on each cpu
spinlock_t lock_kmt;

_Context *kmt_context_save(_Event ev, _Context* context){
    if (cputask[_cpu()]!=NULL) {
       Assert(cputask[_cpu()]->run==1, "running threads run=0");
       kmt->spin_lock(&lock_kmt);
       cputask[_cpu()]->context = *context;
       cputask[_cpu()]->run = 0;
       kmt->spin_unlock(&lock_kmt);
    }
    return NULL;
}

_Context *kmt_context_switch(_Event ev, _Context* context){
    //Scheduler: Randomly selected
    int seed = rand()%MAXTASK;
    //Choose an runnable context
    while (1){
      kmt->spin_lock(&lock_kmt);
      for (int i=0;i<MAXTASK;++i){
        task_t *nxt = tasks[(seed+i)%MAXTASK];
        if (nxt && nxt->run==0){
            Logcontext(nxt);

            nxt->run = 1;
            cputask[_cpu()] = nxt;
            kmt->spin_unlock(&lock_kmt);

            return &cputask[_cpu()]->context;
        }
      }
      kmt->spin_unlock(&lock_kmt);
      //Log("waiting");
    }

    SHOULD_NOT_REACH_HERE();
    return NULL;
}

void kmt_init(){
    //Init spinlock
    kmt->spin_init(&lock_kmt, "kmt");
    //Register irq handler
    os->on_irq(INT_MIN, _EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, _EVENT_NULL, kmt_context_switch);
    //Clear tasks
    for (int i=0;i<MAXTASK;++i){
        tasks[i] = NULL;
    }
    for (int i=0;i<MAXCPU;++i){
        cputask[i] = NULL;
    }
    cnt_tasks = 0;
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    //Check if tasks are full
    if (cnt_tasks>=MAXTASK){
      warning("Create Failed: Task amount overflows\n");
      return 1;
    }
    //Initial task information
    task->stack = pmm->alloc(STACKSIZE);
    if (task->stack==NULL) {
      warning("Create Failed: Memory overflows\n");
      return 1;
    }
    task->name = name;
    task->run = 0;
    task->context = *_kcontext((_Area){task->stack, task->stack+STACKSIZE-1}, entry, arg);
    //Search for a space for the task

    kmt->spin_lock(&lock_kmt);
    cnt_tasks++;
    int i = 0, seed = rand()%MAXTASK;
    while (i<MAXTASK && tasks[(seed+i)%MAXTASK]) i++;
    tasks[(seed+i)%MAXTASK] = task;
    task->id = (seed+i)%MAXTASK;
    kmt->spin_unlock(&lock_kmt);
    //Log("create %d", task->id);

    return 0;
}

void kmt_teardown(task_t *task){
    assert(task!=NULL);
    //Log("free %d", task->id);
    pmm->free(task->stack);

    kmt->spin_lock(&lock_kmt);
    cnt_tasks--;
    tasks[task->id]=NULL;
    kmt->spin_unlock(&lock_kmt);
}


// ============= Spinlock ============

int cpuncli[MAXCPU], intena[MAXCPU];

static void pushcli(void){
    int IF;
    IF = _intr_read();
    _intr_write(0);
    if (cpuncli[_cpu()] == 0) intena[_cpu()] = IF;
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
    lk->cpu = -1;
    lk->locked = 0;
    lk->name = name;
}

void spin_lock(spinlock_t *lk){
    pushcli();
    //printf("L%c %s %d\n","12345678"[_cpu()], lk->name, _intr_read());
    Assert(!holding(lk), "locking a locked lock %s, %d", lk->name, lk->cpu);

    while (_atomic_xchg(&lk->locked, 1)!=0);

    __sync_synchronize();
    lk->cpu = _cpu();
}

void spin_unlock(spinlock_t *lk) {
    Assert(holding(lk), "release an unlocked lock %s, %d", lk->name, lk->cpu);
    lk->cpu = -1;

    __sync_synchronize();

    asm volatile ("movl $0, %0" : "+m" (lk->locked) :);
    //printf("U%c %s %d\n","12345678"[_cpu()], lk->name, _intr_read());
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
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = spin_init,
    .spin_lock = spin_lock,
    .spin_unlock = spin_unlock,
    .sem_init = sem_init,
    .sem_wait = sem_wait,
    .sem_signal = sem_signal,
};
