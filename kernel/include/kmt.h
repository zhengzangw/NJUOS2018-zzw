#ifndef __KMT_H__
#define __KMT_H__

#include <common.h>

// ============= Tread ================

// ============= Spinlock =============

#define cli() asm volatile ("cli" ::: "memory");
#define sti() asm volatile ("sti" ::: "memory");

static inline int readflags(){
    uint eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

#define FL_IF 0x00000200
#define checkIF() if (readflags()&FL_IF) panic("interruptible");

// ============= Sem ==================

#endif
