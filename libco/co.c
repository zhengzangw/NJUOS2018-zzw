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

void *main_stack;
jmp_buf main_env;

struct co {
  char name[64];
  jmp_buf env;
  char stack[32 KB];
  char done;
  void *stack_back;
};
struct co coroutines[MAX_CO];
int co_num;

static inline void changeframe(void * stack){
  asm volatile("mov %0, " SP :
               :
               "g"(stack));
}

void co_init() {
  asm volatile("mov " SP ", %0" :
               "=g"(main_stack));
  coroutines[1].stack[1] = 1;
}

struct co* co_start(const char *name, func_t func, void *arg) {
  printf("======================\n");
  coroutines[co_num].done = 0;
  strcpy(coroutines[co_num].name, name);

  int ind = setjmp(main_env);
  if (!ind){
  asm volatile("mov " SP ", %0; mov %1, " SP :
               "=g"(coroutines[co_num].stack_back):
               "g"(START_OF_STACK(coroutines[co_num].stack)));
   // changeframe(START_OF_STACK(coroutines[co_num].stack));
    //printf("%p %p %p\n",coroutines, coroutines[co_num].stack, START_OF_STACK(coroutines[co_num].stack));
    //printf("*******************\n");
    func(arg); // Test #2 hangs
    coroutines[co_num].done = 1;
  }
  co_num++;
  return &(coroutines[co_num-1]);
}

void co_yield() {
  int id = rand()%(co_num+1);

  int ind = setjmp(coroutines[co_num-1].env);
  if (!ind){
      if (!id){
        changeframe(main_stack);
        longjmp(main_env, 1);
      } else {
        changeframe(START_OF_STACK(coroutines[id-1].stack));
        longjmp(coroutines[id-1].env, 1);
      }
  }
}

void co_wait(struct co *thd) {
    if (!thd->done) {
      co_yield();
    }
}

