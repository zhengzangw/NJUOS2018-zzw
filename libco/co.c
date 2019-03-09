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
  asm volatile("mov " SP ", %0; mov %1, " SP :\
               "=g"(crs[old].stackptr):\
               "g"(crs[new].stackptr))
#define restoreframe(num)\
  asm volatile("mov %0," SP : : "g"(crs[num].stackptr))

void nothing(func_t func, void *arg){
  return;
}

static int times;
#define debug do {\
    printf("id = %d\n", id);
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

  printf("pre = %d\n", pre);
  int ind = setjmp(crs[pre].env);
  printf("ind = %d\n", ind);
  if (!ind){
    changeframe(pre,co_num);
    func_(arg_); // Test #2 hangs
    crs[co_num].done = 1;
  }
  printf("before res: cur = %d!\n", cur);
  restoreframe(cur);

  return &(crs[co_num]);
}

void co_yield() {
  int id = rand()%(co_num+1);
  int pre = cur;
  cur = id;
  printf("id = %d, cur=%d, pre=%d, cur=%d\n", id, cur, pre, cur);
  debug;

  int ind = setjmp(crs[pre].env);
  if (!ind){
        printf("id = %d\n", id);
        changeframe(pre, id);
        printf("id = %d\n", id);
        debug;
        printf("id = %d\n", id);
        longjmp(crs[id].env, 1);
  }
  printf("bef res, cur = %d\n", cur);
  restoreframe(cur);
  //printf("End of yield\n");
}

void co_wait(struct co *thd) {
    printf("wait\n");
    while (!thd->done){
        co_yield();
    }
}

