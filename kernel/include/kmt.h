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
  int run, id;
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
