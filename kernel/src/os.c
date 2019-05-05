#include <common.h>
#include <os.h>

#ifdef DEBUG_LOCK
spinlock_t lock_debug;
#endif

//Test
void logging(void *arg) {
  while (1){
    Log("%s", (char *)arg);
  }
}

static void os_init() {
  pmm->init();
  kmt->init();
  dev->init();

  #ifdef DEBUG_LOCK
  kmt->spin_init(&lock_debug, "debug");
  #endif

  char *str = "test";
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-1", logging, str);
}


static void os_run() {
  assertIF0();
  Logcpu();
  _intr_write(1);
  while (1) {
    _yield();
  }
}

callback_t handlers[MAXCB];
int h_handlers;

static _Context *os_trap(_Event ev, _Context *context) {
  Log("%d", ev.event);
  _Context *ret = NULL;
  for (int i=0;i<h_handlers;++i){
      if (handlers[i].event == _EVENT_NULL || handlers[i].event == ev.event){
          _Context *next = handlers[i].handler(ev, context);
          if (next) ret = next;
      }
  }
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
