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

#if defined(__i386__)
  #define SP "%%esp"
  #define BP "%%ebp"
#elif defined(__x86_64__)
  #define SP "%%rsp"
  #define BP "%%rbp"
#endif

struct co {
  char name[64];
  jmp_buf env;
  char done;
  void *stackptr;
  char stack[32 KB];
};
struct co crs[MAX_CO];
int co_num, cur;
func_t func_;
void * arg_;

#define changeframe(old, new)\
  asm volatile("mov " SP ", %0" :\
               "=g"(crs[old].stackptr));\
  asm volatile("mov %0, " SP :\
               :\
               "g"(crs[new].stackptr))
#define restoreframe(num)\
  asm volatile("mov %0," SP : : "g"(crs[num].stackptr))

void nothing(func_t func, void *arg){
  return;
}

//static int times;
#define debug do {\
    printf("DEBUG #%d\n", ++times);\
    void* sp;\
    asm volatile("mov " SP ", %0": "=g"(sp));\
    printf("SP = %p\n", sp);\
    for (int i=0;i<3;++i){\
        printf("stackptr %d: %p\n", i, crs[i].stackptr);\
    }\
    printf("\n");\
} while (0);

void co_init() {
  strcpy(crs[0].name, "main");
  crs[0].stackptr = NULL;
  co_num = cur = 0;
}

struct co* co_start(const char *name, func_t func, void *arg) {
  func_ = func; arg_ = arg;

  crs[++co_num].done = 0;
  crs[co_num].stackptr = START_OF_STACK(crs[co_num].stack);
  strcpy(crs[co_num].name, name);
  int pre = cur;
  cur = co_num;

  int ind = setjmp(crs[pre].env);
  if (!ind){
    changeframe(pre,co_num);
    func_(arg_); // Test #2 hangs
    printf("Alert!\n");
    crs[co_num].done = 1;
    co_yield();
  }
  restoreframe(cur);

  return &(crs[co_num]);
}

void co_yield() {
  int pre = cur;
  do {
    cur = rand()%(co_num+1);
  } while (!crs[cur].done);
  printf("cur = %d\n", cur);

  int ind = setjmp(crs[pre].env);
  //printf("ind = %d, pre = %d, cur = %d\n", ind, pre, cur);
  if (!ind){
        changeframe(pre, cur);
  //printf("cur = %d\n", cur);
        longjmp(crs[cur].env, 1);
  }
  restoreframe(cur);
}

void co_wait(struct co *thd) {
    printf("wait\n");
    while (!thd->done){
        int ind = setjmp(crs[cur].env);
        if (!ind){
          co_yield();
        }
    }
}

