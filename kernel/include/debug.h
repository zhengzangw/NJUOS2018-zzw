#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <klib.h>

#define DEBUG
#define DEBUG_LOCK
//#define DEBUG_PMM

// ========== General ============

typedef unsigned int uint;

#ifdef DEBUG_LOCK
extern spinlock_t lock_debug;
#endif

#define Logcpu() Log("cpu #%c:\n", "12345678"[_cpu()])

#ifndef DEBUG
#define Log(format, ...)
#else
#ifndef DEBUG_LOCK
#define Log(format, ...) \
    printf("\33[1;34m[cpu #%c,%s,%d,%s] " format "\33[0m\n", \
           "12345678"[_cpu()],  __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Log(format, ...) \
    kmt->spin_lock(&lock_debug); \
    printf("\33[1;34m[cpu #%c,%s,%d,%s] " format "\33[0m\n", \
           "12345678"[_cpu()],  __FILE__, __LINE__, __func__, ## __VA_ARGS__)
    kmt->spin_unlock(&lock_debug)
#endif
#endif

#define Logint(x) \
    Log(#x " = %d", x)

#define Assert(cond, ...) \
    do { \
        if (!(cond)) { \
            Log(__VA_ARGS__); \
            Log("Assertion fail"); \
            _halt(1); \
            } \
       } while (0)

#define Panic(format, ...) \
    Assert(0, format, ## __VA_ARGS__)

#define TODO() Panic("please implement me")

// ========== LIST ===========
#define Lognode(tag, node) Log(tag "node: start=%p, end=%p", node->start, node->end)
// ========== SPINLOCK =======
#define FL_IF 0x00000200
#define assertIF0() Assert((readflags()&FL_IF)==0, "interruptible where IF should be 0")
#define assertIF1() Assert((readflags()&FL_IF)!=0, "noninterruptible where IF should be 1")
#define readflags() (int (*_readflags)){
    uint eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}();

#endif
