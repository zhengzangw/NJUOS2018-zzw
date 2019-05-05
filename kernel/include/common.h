#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>

struct task {};
struct spinlock {
  int locked;
  //For debugging
  int cpu;
};
struct semaphore {};

typedef unsigned int uint;

#endif
