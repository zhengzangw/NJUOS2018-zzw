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

#define mainco coroutines[0]

struct co {
  char name[64];
  jmp_buf env;
  char stack[32 KB];
  char done;
  void *stack_back;
};
struct co coroutines[MAX_CO];
int co_num;

#define changeframe(co_num)\
  asm volatile("mov " SP ", %0; mov %1, " SP :\
               "=g"(coroutines[co_num].stack_back):\
               "g"(START_OF_STACK(coroutines[co_num].stack)))
#define restoreframe(co_num)\
  asm volatile("mov %0," SP : : "g"(coroutines[co_num].stack_back))


void co_init() {
  strcpy(coroutines[0].name, "main");
  co_num = 0;
}

struct co* co_start(const char *name, func_t func, void *arg) {
  coroutines[++co_num].done = 0;
  strcpy(coroutines[co_num].name, name);

  int ind = setjmp(mainco.env);
  printf("ind=%d\n", ind);
  if (!ind){
    changeframe(co_num);
    func(arg); // Test #2 hangs
    coroutines[co_num].done = 1;
    restoreframe(co_num);
  }
  return &(coroutines[co_num-1]);
}

void co_yield() {
  int id = rand()%(co_num+1);

  int ind = setjmp(coroutines[id].env);
  if (!ind){
        printf("change to id=%d\n", id);
        changeframe(id);
        longjmp(coroutines[id].env, 1);
        restoreframe(id);
  }
}

void co_wait(struct co *thd) {
    if (!thd->done) {
      co_yield();
    }
}

