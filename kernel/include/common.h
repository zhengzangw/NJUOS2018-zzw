#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>

typedef unsigned int uint;

struct task {};
struct spinlock {
  int locked;
  //For debugging
  uint cpu;
};
void initlock(struct spinlock *lk);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
struct semaphore {};

#endif
