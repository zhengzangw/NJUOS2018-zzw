#ifndef __KMT_H__
#define __KMT_H__

#include <common.h>
#include <klib.h>

// ============= THREAD ================

#define STACKSIZE 4096
#define MAXTASK 256
#define empty(taskptr) (taskptr==NULL||taskptr->exists==0)
struct task {
  int exists, run;
  const char *name;
  _Context context;
  char* stack;
};

// ============= SPINLOCK =============

struct spinlock {
  const char *name;
  int locked;
  int cpu;
};

// ============= SEMAPHORE ==================

struct semaphore {};

#endif
