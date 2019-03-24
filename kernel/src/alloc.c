#include <common.h>
#include <klib.h>
#include <lock.h>

#define DEBUG
#define Lognode(node) printf("Node: start=%p, end=%p\n", node->start, node->end)
//#define CORRECTNESS_FIRST

static uintptr_t pm_start, pm_end, start;
static lock_t alloc_lock;

struct node {
  struct node *next, *pre;
  uintptr_t start, end;
};
struct node *head, *tail;

static void pmm_init() {
  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;
  start = pm_start;

  init(&alloc_lock);

  head = (void *)pm_start;
  tail = (void *)(pm_end-sizeof(struct node));
  head->next = tail; head->pre = NULL;
  tail->next = NULL; tail->pre = head;
  head->start = pm_start; head->end = head->start+sizeof(struct node);
  tail->start = pm_end-sizeof(struct node); tail->end = tail->start+sizeof(struct node);

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
      if (p->next->start-p->end>=size+sizeof(struct node)){
        tmp = (void *)p->end;
        tmp->start = p->end;
        tmp->end = tmp->start + size;
        tmp->pre = p;
        tmp->next = p->next;
        p->next->pre = tmp;
        p->next = tmp;
        flag = 1;
        ret = (void *)(tmp->start + sizeof(struct node));
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

#endif
}

MODULE_DEF(pmm) {
  .init = pmm_init,
  .alloc = kalloc,
  .free = kfree,
};
