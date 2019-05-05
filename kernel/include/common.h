#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>

typedef unsigned int uint;

struct task {};
struct spinlock {
  uint locked;
  //For debugging
  uint cpu;
  uint pcs[10];
};
void initlock(struct spinlock *lk);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
struct semaphore {};

#endif
