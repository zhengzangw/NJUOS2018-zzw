#ifndef __KMT_H__
#define __KMT_H__

#include <common.h>

// ============= THREAD ================

// ============= SPINLOCK =============

#define cli() asm volatile ("cli" ::: "memory");
#define sti() asm volatile ("sti" ::: "memory");

static inline int readflags(){
    uint eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

// ============= SEMAPHORE ==================

#endif
