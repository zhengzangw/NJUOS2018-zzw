#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>

struct task {};
struct spinlock {
  int locked;
  const char *name;
  int cpu;
};
struct semaphore {};

typedef unsigned int uint;

#endif
