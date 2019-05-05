#include <common.h>
#include <klib.h>
#include <list.h>
#include <debug.h>

//#define CORRECTNESS_FIRST

#ifdef CORRECTNESS_FIRST
static uintptr_t start;
#endif

static uintptr_t pm_start, pm_end;
static spinlock_t alloc_lock;

static void pmm_init() {
  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;
  kmt->spin_init(&alloc_lock, "alloc");

#ifdef CORRECTNESS_FIRST
  start = pm_start;
#else
  list_init(pm_start, pm_end);
#endif

#ifdef DEBUG_PMM
  Log("pm_start = %p\tpm_end = %p\tsize of heap=%p", pm_start, pm_end, pm_end-pm_start);
#endif
}

static void *kalloc(size_t size) {
    void *ret=NULL;
    kmt->spin_lock(&alloc_lock);

#ifdef CORRECTNESS_FIRST

  #ifdef DEBUG_PMM
    Log("cpu = %c, malloc (%p,%p)", "12345678"[_cpu()], start, start+size);
  #endif

  if (start+size >= pm_end) {
    printf("No enough space. FAIL!\n");
  } else {
    start += size;
    ret = (void *)start;
  }

#else

    for (struct node*p=head;p!=tail;p=p->next){
      Assert(p->next->pre==p, "%p != %p\n", p->next->pre, p);
      if (p->next->start-p->end>=size+BIAS){
        ret = add_node(p, size);

#ifdef DEBUG_PMM
      Log("cpu = %c, malloc (%p,%p)\n", "12345678"[_cpu()], p->next->start, p->next->end);
#endif
        break;
      }
    }

    if (ret==NULL){
      printf("No enough space. FAIL!\n");
    }

#endif
    kmt->spin_unlock(&alloc_lock);
return ret;
}

static void kfree(void *ptr) {
#ifdef CORRECTNESS_FIRST
  return;
#else
  struct node *p = (struct node *)((uintptr_t)ptr - BIAS);
  kmt->spin_lock(&alloc_lock);
  Assert(p->next->pre==p, "%p != %p\n", p->next->pre, p);
  #ifdef DEBUG
    Log("free %p: ", ptr);
    Lognode(p);
  #endif
  delete_node(p);
  kmt->spin_unlock(&alloc_lock);
#endif
}

MODULE_DEF(pmm) {
  .init = pmm_init,
  .alloc = kalloc,
  .free = kfree,
};
