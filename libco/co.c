#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include "co.h"
#include <assert.h>

#define KB * 1024LL
#define MB KB * 1024LL
#define GB MB * 1024LL
#define MAX_CO 10
#define START_OF_STACK(stack) ((stack)+sizeof(stack))

#ifdef DEBUG
#define Log(format, ...) \
        printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
                        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#define Assert(cond, ...) \
      do { \
          if (!(cond)) { \
                Log(__VA_ARGS__); \
                assert(cond); \
              } \
        } while (0)
#define panic(format, ...) \
      Assert(0, format, ## __VA_ARGS__)
#define TODO() panic("please implement me")
static int times;
#define debug() do {\
    Log("DEBUG #%d", ++times);\
    void* sp;\
    asm volatile("mov " SP ", %0": "=g"(sp));\
    Log("SP = %p", sp);\
    for (int i=0;i<3;++i){\
        Log("stackptr %d: %p", i, crs[i].stackptr);\
    }\
} while (0);
#else
#define Log(format, ...)
#define Assert()
#define panic()
#define TODO()
#define debug()
#endif

#if defined(__i386__)
#define SP "%%esp"
#define BP "%%ebp"
#elif defined(__x86_64__)
#define SP "%%rsp"
#define BP "%%rbp"
#endif

#define changeframe(old, new)\
  asm volatile("mov " SP ", %0" :\
               "=g"(crs[old].stackptr));\
  asm volatile("mov %0, " SP :\
               :\
               "g"(crs[new].stackptr))
#define restoreframe(num)\
  asm volatile("mov %0," SP : : "g"(crs[num].stackptr))

struct co {
        char name[64];
        jmp_buf env;
        char done;
        void *stackptr;
        char stack[32 KB] __attribute__ (( aligned(16) ));
};
struct co crs[MAX_CO];
int co_num, cur;

//Variable storing local variable
func_t func_;
void *arg_;

void co_init()
{
        strcpy(crs[0].name, "main");
        crs[0].stackptr = NULL;
        co_num = cur = 0;
}

struct co *co_start(const char *name, func_t func, void *arg)
{
        func_ = func;
        arg_ = arg;

        co_num++;
        strcpy(crs[co_num].name, name);
        crs[co_num].done = 0;
        crs[co_num].stackptr = START_OF_STACK(crs[co_num].stack);

        int ind = setjmp(crs[cur].env);
        if (!ind) {
                changeframe(cur, co_num);
                cur = co_num;
                func_(arg_);    // Test #2 hangs
                crs[cur].done = 1;
                co_yield();
        }
        restoreframe(cur);

        return &(crs[co_num]);
}

void co_yield()
{
        int pre = cur;
        do {
                cur = rand() % (co_num + 1);
        } while (crs[cur].done);

        int ind = setjmp(crs[pre].env);
        debug();
        if (!ind) {
                changeframe(pre, cur);
                longjmp(crs[cur].env, 1);
        }
        restoreframe(cur);
}

void co_wait(struct co *thd)
{
        while (!thd->done) {
                int ind = setjmp(crs[cur].env);
                if (!ind) {
                        co_yield();
                }
        }
}
