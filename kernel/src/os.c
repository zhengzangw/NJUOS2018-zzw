#include <common.h>
#include <klib.h>

#ifdef DEBUG_LOCK
spinlock_t lock_debug;
#endif

spinlock_t lock_test;

static void os_init() {
  pmm->init();
  kmt->init();
  dev->init();

  kmt->spin_init(&lock_test, "test");
  #ifdef DEBUG_LOCK
  kmt->spin_init(&lock_debug, "debug");
  #endif
}




static void os_run() {
  hello();
  _intr_write(1);
  while (1) {
    _yield();
  }
}

static _Context *os_trap(_Event ev, _Context *context) {
  _Context *ret = NULL;
  return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
}

MODULE_DEF(os) {
  .init   = os_init,
  .run    = os_run,
  .trap   = os_trap,
  .on_irq = os_on_irq,
};
