#ifndef __KMT_H__
#define __KMT_H__

#include <common.h>
#include <klib.h>

#define MAXCPU 64

// ============= THREAD ================

#define STACKSIZE 4096
#define MAXTASK 64
#define empty(taskptr) (taskptr==NULL||taskptr->exists==0)
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX-1)
extern spinlock_t lock_kmt;
struct task {
  int exists, run, id;
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
