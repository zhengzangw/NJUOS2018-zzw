#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>
#include <debug.h>

struct task {
  const char *name;
  _Context context;
  char stack[4096];
};
struct spinlock {
  const char *name;
  int locked;
  int cpu;
};
struct semaphore {};

#endif
