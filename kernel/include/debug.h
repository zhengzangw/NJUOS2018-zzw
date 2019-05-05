#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <klib.h>

#define DEBUG
#define DEBUG_LOCK
#define DEBUG_PMM

// ========== General ============

#ifdef DEBUG_LOCK
extern spinlock_t lock_debug;
#endif

#ifndef DEBUG
#define Log(format, ...)
#else
#ifndef DEBUG_LOCK
#define Log(format, ...) \
    printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
          __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Log(format, ...) \
    kmt->spin_lock(&lock_debug); \
    printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
          __FILE__, __LINE__, __func__, ## __VA_ARGS__); \
    kmt->spin_unlock(&lock_debug)
#endif
#endif

#define Logint(x) \
    Log(#x " = %d", x)

#define Assert(cond, ...) \
    do { \
        if (!(cond)) { \
            Log(__VA_ARGS__); \
            assert(cond); \
            } \
       } while (0)

#define Panic(format, ...) \
    Assert(0, format, ## __VA_ARGS__)

#define TODO() Panic("please implement me")

#define Logcpu() Log("cpu #%c:\n", "12345678"[_cpu()])

// ========== LIST ===========
#define Lognode(tag, node) Log(tag "node: start=%p, end=%p\n", node->start, node->end)
// ========== SPINLOCK =======
#define FL_IF 0x00000200
#define assertIF1() Assert((readflags()&FL_IF)==0, "interruptible where IF should be 0")
#define assertIF0() Assert((readflags()&FL_IF)==1, "noninterruptible where IF should be 1")

#endif
