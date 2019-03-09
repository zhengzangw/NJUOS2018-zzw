#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include "co.h"

#define KB * 1024LL
#define MB KB * 1024LL
#define GB MB * 1024LL
#define MAX_CO 10
#define START_OF_STACK(stack) ((stack)+sizeof(stack))

#if defined(__i386__)
  #define SP "%%esp"
#elif defined(__x86_64__)
  #define SP "%%rsp"
#endif

struct co {
  char name[64];
  jmp_buf env;
  char stack[32 KB];
  char done;
  void *stack_back;
};
struct co coroutines[MAX_CO];
int co_num, cur_co;

#define mainco coroutines[0]
#define changeframe(num)\
  asm volatile("mov " SP ", %0; mov %1, " SP :\
               "=g"(coroutines[num].stack_back):\
               "g"(START_OF_STACK(coroutines[num].stack)))
#define restoreframe(num)\
  asm volatile("mov %0," SP : : "g"(coroutines[num].stack_back))


void co_init() {
  strcpy(coroutines[0].name, "main");
  co_num = cur_co = 0;
}

struct co* co_start(const char *name, func_t func, void *arg) {
  coroutines[++co_num].done = 0;
  strcpy(coroutines[co_num].name, name);

  int ind = setjmp(mainco.env);
  printf("ind=%d\n", ind);
  if (!ind){
    cur_co = co_num;
    changeframe(co_num);
    func(arg); // Test #2 hangs
    coroutines[co_num].done = 1;
    restoreframe(co_num);
  }
  return &(coroutines[co_num-1]);
}

void co_yield() {
  int id = rand()%(co_num+1);

  int ind = setjmp(coroutines[cur_co].env);
  if (!ind){
        cur_co = id;
        changeframe(id);
        longjmp(coroutines[id].env, 1);
  }
}

void co_wait(struct co *thd) {
    if (!thd->done) {
      co_yield();
    }
}

