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

#define Logcpu() Log("cpu #%c:", "12345678"[_cpu()])

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
           "12345678"[_cpu()],  __FILE__, __LINE__, __func__, ## __VA_ARGS__); \
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
#define SHOULD_NOT_REACH_HERE() Panic("Should Not Reach Here!")

// ========== LIST ===========
#define Lognode(tag, node) Log(tag "node: start=%p, end=%p", node->start, node->end)
// ========== SPINLOCK =======
extern int cpuncli[64];
#define assertIF0() Assert(_intr_read()==0, "IF should be 0")
#define assertIF1() Assert(_intr_read()!=0, "IF should be 1")
#define Logintr() printf("IF=%d", _intr_read())
#define Loglock(lock) printf("lock=%s", lock->name);
// ========== THREAD =========
#define Logcontext(context) Log("id=%d, run=%d, name=%s", context->id, context->run, context->name)
#endif
