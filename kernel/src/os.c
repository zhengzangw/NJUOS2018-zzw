#include <common.h>
#include <os.h>

#ifdef DEBUG_LOCK
spinlock_t lock_debug;
#endif

spinlock_t lock_test;

//Test
static void hello() {
  kmt->spin_lock(&lock_test);
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++){
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
  kmt->spin_unlock(&lock_test);
}

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
  assertIF0();
  hello();
  _intr_write(1);
  while (1) {
    _yield();
  }
}

callback_t handlers[MAXCB];
int h_handlers;

static _Context *os_trap(_Event ev, _Context *context) {
  _Context *ret = context;
  for (int i=0;i<h_handlers;++i){
      if (handlers[i].event == _EVENT_NULL || handlers[i].event == ev.event){
          _Context *next = handlers[i].handler(ev, context);
          if (next) ret = next;
      }
  }
  Log("%d", ev.event);
  return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
    handlers[h_handlers++] = (callback_t){handler, event, seq};
    int i = h_handlers-1;
    while (i&&handlers[i].seq<=handlers[i-1].seq){
        callback_t tmp = handlers[i-1];
        handlers[i-1] = handlers[i];
        handlers[i] = tmp;
    }
}

MODULE_DEF(os) {
  .init   = os_init,
  .run    = os_run,
  .trap   = os_trap,
  .on_irq = os_on_irq,
};
