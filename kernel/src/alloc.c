#include <common.h>
#include <klib.h>
#include "lock.h"

static uintptr_t pm_start, pm_end;
static lock_t debug_lock;

static void pmm_init() {
  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;

  init(&debug_lock);

  lock(&debug_lock);
  printf("pm_start = %p\npm_end = %p\nsize of heap=%p\n", pm_start, pm_end, pm_end-pm_start);
  unlock(&debug_lock);
}

static void *kalloc(size_t size) {
  return NULL;
}

static void kfree(void *ptr) {
}

MODULE_DEF(pmm) {
  .init = pmm_init,
  .alloc = kalloc,
  .free = kfree,
};
