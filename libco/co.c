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
  void *stackptr;
};
struct co crs[MAX_CO];
int co_num, cur;

#define changeframe(old, new)\
  asm volatile("mov " SP ", %0; mov %1, " SP :\
               "=g"(crs[old].stackptr):\
               "g"(crs[new].stackptr))
#define restoreframe(num)\
  asm volatile("mov %0," SP : : "g"(crs[num].stackptr))


void co_init() {
  strcpy(crs[0].name, "main");
  co_num = cur = 0;
}

struct co* co_start(const char *name, func_t func, void *arg) {
  crs[++co_num].done = 0;
  strcpy(crs[co_num].name, name);

  int ind = setjmp(crs[cur].env);
  if (!ind){
    changeframe(cur,co_num);
    cur = co_num;
    func(arg); // Test #2 hangs
    crs[co_num].done = 1;
  }
  restoreframe(cur);

  return &(crs[co_num]);
}

void co_yield() {
  int id = rand()%(co_num+1);

  int ind = setjmp(crs[cur].env);
  printf("ind = %d\n", ind);
  if (!ind){
        changeframe(cur, id);
        cur = id;
        longjmp(crs[id].env, 1);
  }
  restoreframe(cur);
  //printf("End of yield\n");
}

void co_wait(struct co *thd) {
    printf("wait\n");
    while (!thd->done){
        co_yield();
    }
}

