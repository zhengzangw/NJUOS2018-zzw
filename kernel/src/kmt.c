#include <common.h>
#include <kmt.h>

// ============= Thread   ============

task_t *tasks[MAXTASK];
int cnt_tasks;            // total created tasks
task_t *cputask[MAXCPU];  // task running on each cpu
task_t *cputask_last[MAXCPU];
task_t cpudefaulttask[MAXCPU];
int notdefault[MAXCPU];
spinlock_t lock_kmt;

_Context *kmt_context_save(_Event ev, _Context *context) {
    kmt->spin_lock(&lock_kmt);
    if (cputask_last[_cpu()]!=NULL && cputask_last[_cpu()]!=cputask[_cpu()]){
        cputask_last[_cpu()]->run = 0;
    }
    if (!notdefault[_cpu()]) {
        cpudefaulttask[_cpu()].context = *context;
        notdefault[_cpu()] = 1;
    }
    if (cputask[_cpu()] != NULL) {
        Assert(cputask[_cpu()]->sleep == 1 || cputask[_cpu()]->run == 1,
               "running threads run=0, %s, id=%d", cputask[_cpu()]->name,
               cputask[_cpu()]->id);
        cputask[_cpu()]->context = *context;
    }
    cputask_last[_cpu()] = cputask[_cpu()];
    kmt->spin_unlock(&lock_kmt);
    return NULL;
}

_Context *kmt_context_switch(_Event ev, _Context *context) {
    // Scheduler: Linear selected
    int seed;
    if (cputask[_cpu()])
        seed = cputask[_cpu()]->id;
    else
        seed = rand() % MAXTASK;
    // Choose an runnable context
    task_t *ret = NULL;
    kmt->spin_lock(&lock_kmt);
    for (int i = 0; i < MAXTASK; ++i) {
        task_t *nxt = tasks[(seed + i + 1) % MAXTASK];
        Logcontext(nxt);
        if (nxt && nxt->run == 0 && nxt->sleep == 0) {
            ret = nxt;
            break;
        }
    }
    Log("=============");
    if (ret == NULL) {
        ret = &cpudefaulttask[_cpu()];
    }
    ret->run = 1;
    cputask[_cpu()] = ret;
    _Context *retct = &ret->context;
    kmt->spin_unlock(&lock_kmt);

    return retct;
}

void kmt_init() {
    // Init spinlock
    kmt->spin_init(&lock_kmt, "kmt");
    // Register irq handler
    os->on_irq(INT_MIN, _EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, _EVENT_NULL, kmt_context_switch);
    // Clear tasks
    for (int i = 0; i < MAXTASK; ++i) {
        tasks[i] = NULL;
    }
    for (int i = 0; i < MAXCPU; ++i) {
        cputask[i] = cputask_last[i] = NULL;
        cpudefaulttask[i].run = 0;
        cpudefaulttask[i].sleep = 0;
        cpudefaulttask[i].name = "cpudefault";
    }
    cnt_tasks = 0;
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg),
               void *arg) {
    // Check if tasks are full
    if (cnt_tasks >= MAXTASK) {
        warning("Create Failed: Task amount overflows\n");
        return 1;
    }
    // Initial task information
    task->stack = pmm->alloc(STACKSIZE);
    if (task->stack == NULL) {
        warning("Create Failed: Memory overflows\n");
        return 1;
    }
    task->name = name;
    task->run = task->sleep = 0;
    task->context = *_kcontext(
        (_Area){task->stack, task->stack + STACKSIZE - 1}, entry, arg);
    // Search for a space for the task

    kmt->spin_lock(&lock_kmt);
    cnt_tasks++;
    int i = 0, seed = rand() % MAXTASK;
    while (i < MAXTASK && tasks[(seed + i) % MAXTASK]) i++;
    tasks[(seed + i) % MAXTASK] = task;
    task->id = (seed + i) % MAXTASK;
    kmt->spin_unlock(&lock_kmt);
    // Log("create %d", task->id);

    return 0;
}

void kmt_teardown(task_t *task) {
    assert(task != NULL);
    // Log("free %d", task->id);
    pmm->free(task->stack);

    kmt->spin_lock(&lock_kmt);
    cnt_tasks--;
    tasks[task->id] = NULL;
    kmt->spin_unlock(&lock_kmt);
}

// ============= Spinlock ============

int cpuncli[MAXCPU], intena[MAXCPU];

static void pushcli(void) {
    int IF;
    IF = _intr_read();
    _intr_write(0);
    if (cpuncli[_cpu()] == 0) intena[_cpu()] = IF;
    cpuncli[_cpu()] += 1;
}

static void popcli(void) {
    assertIF0();
    if (--cpuncli[_cpu()] < 0) Panic("popcli");
    if (cpuncli[_cpu()] == 0 && intena[_cpu()]) _intr_write(1);
}

static int holding(spinlock_t *lock) {
    int r;
    pushcli();
    r = lock->locked && lock->cpu == _cpu();
    popcli();
    return r;
}

void spin_init(spinlock_t *lk, const char *name) {
    lk->cpu = -1;
    lk->locked = 0;
    lk->name = name;
}

void spin_lock(spinlock_t *lk) {
    pushcli();
    // printf("L%c %s %d\n","12345678"[_cpu()], lk->name, _intr_read());
    // Assert(!holding(lk), "locking a locked lock %s, %d", lk->name, lk->cpu);
    assert(!holding(lk));

    while (_atomic_xchg(&lk->locked, 1) != 0)
        ;

    __sync_synchronize();
    lk->cpu = _cpu();
}

void spin_unlock(spinlock_t *lk) {
    assert(holding(lk));
    lk->cpu = -1;

    __sync_synchronize();

    asm volatile("movl $0, %0" : "+m"(lk->locked) :);
    // printf("U%c %s %d\n","12345678"[_cpu()], lk->name, _intr_read());
    popcli();
}

// ============= Semaphore ==============

void sem_init(sem_t *sem, const char *name, int value) {
    sem->name = name;
    sem->count = value;
    sem->cnt_tasks = 0;
    kmt->spin_init(&sem->lock, name);
}

#define head sem->pcb
void sem_list_add(sem_t *sem, task_t *task) {
    tasknode_t *tasknode = pmm->alloc(sizeof(tasknode_t));
    tasknode->task = task;
    if (head == NULL) {
        tasknode->nxt = tasknode->pre = NULL;
        head = tasknode;
    } else {
        assert(head->pre == NULL);
        if (head->nxt) {
            Assert(head->nxt->pre == head, "head->nxt: %d (%p)!=(%p) head: %d",
                   head->nxt->task->id, head->nxt->pre, head, head->task->id);
        }
        tasknode->nxt = head;
        tasknode->pre = head->pre;
        head->pre = tasknode;
        head = tasknode;
    }
    assert(head);
}

void sem_list_delete(sem_t *sem) {
    tasknode_t *ptr= head;
    while (ptr->nxt!=NULL) ptr=ptr->nxt;
    ptr->task->sleep = 0;

    if (ptr==head) head=head->nxt;
    else ptr->pre->nxt = ptr->nxt;
    if (ptr && ptr->nxt) {
        Assert(head->nxt->pre == head, "head->nxt: %d %d (%p)!=(%p) head: %d",
               head->nxt, head->nxt->task->id, head->nxt->pre, head,
               head->task->id);
        ptr->nxt->pre = ptr->pre;
    }
    pmm->free(ptr);
}

void sem_wait(sem_t *sem) {
    kmt->spin_lock(&sem->lock);
    //Log("sem_wait %s:%d, cnt:%d", sem->name, sem->count, sem->cnt_tasks);
    sem->count--;
    if (sem->count < 0) {
        sem->cnt_tasks++;
        cputask[_cpu()]->sleep = 1;
        sem_list_add(sem, cputask[_cpu()]);
        assert(sem->pcb);
        assert(cputask[_cpu()]->run==1);
        //Log("waiting");
        kmt->spin_unlock(&sem->lock);

        _yield();
    } else {
        kmt->spin_unlock(&sem->lock);
    }
}
void sem_signal(sem_t *sem) {
    kmt->spin_lock(&sem->lock);
    if (sem->cnt_tasks > 0) {
        Assert(sem->pcb, "%s: no head, cnt=%d", sem->name, sem->cnt_tasks);
        sem_list_delete(sem);
        sem->cnt_tasks--;
    }
    sem->count++;
//Log("sem_sigal %s:%d, cnt:%d", sem->name, sem->count, sem->cnt_tasks);
    kmt->spin_unlock(&sem->lock);
}

MODULE_DEF(kmt){
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
