#include <common.h>
#include <klib.h>
#include <lock.h>
#include <list.h>

#define DEBUG
//#define CORRECTNESS_FIRST
#ifdef CORRECTNESS_FIRST
static uintptr_t start;
#endif

static uintptr_t pm_start, pm_end;
static lock_t alloc_lock;

static void pmm_init() {
  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;
  //start = pm_start;

  init(&alloc_lock);

  list_init(pm_start, pm_end);
  

#ifdef DEBUG
  lock(&alloc_lock);
  Lognode(head);
  Lognode(tail);
  printf("pm_start = %p\npm_end = %p\nsize of heap=%p\n", pm_start, pm_end, pm_end-pm_start);
  unlock(&alloc_lock);
#endif
}

static void *kalloc(size_t size) {
void *ret;
lock(&alloc_lock);
#ifdef CORRECTNESS_FIRST
  
  #ifdef DEBUG
    printf("cpu = %c, malloc (%p,%p)\n", "12345678"[_cpu()], start, start+size);
  #endif

  if (start+size >= pm_end) {
    printf("No enough space. FAIL!\n");
    ret = NULL;
  } else {
    start += size;
    ret = (void *)start;
  }

#else

    char flag = 0;
    struct node *tmp;
    for (struct node*p=head;p!=tail;p=p->next){
      assert(p->next->pre==p);
      if (p->next->start-p->end>=size+BIAS){
        tmp = (void *)p->end;
        tmp->start = p->end;
        tmp->end = tmp->start + size+BIAS;
        tmp->pre = p;
        tmp->next = p->next;
        p->next->pre = tmp;
        p->next = tmp;
        flag = 1;
        ret = (void *)(tmp->start + BIAS);
        printf("cpu = %c, malloc (%p,%p)\n", "12345678"[_cpu()], tmp->start, tmp->end);
        break;
      }
    }

    if (!flag){
      printf("No enough space. FAIL!\n");
      ret = NULL;
    }
  
#endif
unlock(&alloc_lock);
return ret;
}

static void kfree(void *ptr) {
#ifdef CORRECTNESS_FIRST
  return;
#else
lock(&alloc_lock);
  printf("free %p: ", ptr);
  struct node *p = (struct node *)((uintptr_t)ptr - BIAS);
  Lognode(p);
  p->next->pre = p->pre;
  p->pre->next = p->next;
unlock(&alloc_lock);
#endif
}

MODULE_DEF(pmm) {
  .init = pmm_init,
  .alloc = kalloc,
  .free = kfree,
};
