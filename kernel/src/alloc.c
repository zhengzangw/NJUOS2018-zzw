#include <common.h>
#include <klib.h>
#include <lock.h>

#define CORRECTNESS_FIRST

static uintptr_t pm_start, pm_end, start;
static lock_t debug_lock;

static void pmm_init() {
  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;
  start = pm_start;

  init(&debug_lock);

  lock(&debug_lock);
  printf("pm_start = %p\npm_end = %p\nsize of heap=%p\n", pm_start, pm_end, pm_end-pm_start);
  unlock(&debug_lock);
}

static void *kalloc(size_t size) {
#ifdef CORRECTNESS_FIRST
  void *ret;
  lock(&debug_lock);
    printf("cpu = %c, malloc (%p,%p)\n", "12345678"[_cpu()], start, start+size);
    if (start+size >= pm_end) {
      printf("No enough space. FAIL!\n");
      *ret = NULL;
    } else {
      start += size;
      *ret = (void *)start;
    }
  unlock(&debug_lock);
  return ret;
#else

#endif
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
