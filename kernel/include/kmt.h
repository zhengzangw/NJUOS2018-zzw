#ifndef __KMT_H__
#define __KMT_H__

#include <common.h>
#include <klib.h>
#include <vfs.h>

#define MAXCPU 64

// ============= THREAD ================

#define STACKSIZE 9182
#define MAXTASK 1024
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX-1)
#define NOFILE 64
struct task {
  const char *name;
  _Context context;
  int run, id, sleep;
  char* stack;
  file_t *flides[NOFILE];
};
extern task_t *cputask[MAXCPU];  // task running on each cpu

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
