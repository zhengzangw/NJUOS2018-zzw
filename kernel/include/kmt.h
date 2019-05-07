#ifndef __KMT_H__
#define __KMT_H__

#include <common.h>
#include <klib.h>

#define MAXCPU 64

// ============= THREAD ================

#define STACKSIZE 4096
#define MAXTASK 64
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX-1)
struct task {
  const char *name;
  _Context context;
  int run, id, runnable;
  char* stack;
};

// ============= SPINLOCK =============

struct spinlock {
  const char *name;
  int locked;
  int cpu;
};

// ============= SEMAPHORE ==================

typedef struct Tasknode {
  task_t *task;
  struct Tasknode *nxt, *pre;
} tasknode_t;

struct semaphore {
  const char *name;
  int count, cnt_tasks;
  spinlock_t lock;
  tasknode_t *pcb;
};

#endif
