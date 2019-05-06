#include <common.h>
#include <os.h>

#ifdef DEBUG_LOCK
spinlock_t lock_debug;
#endif
spinlock_t lock_print;
spinlock_t lock_os;

//Test
void logging(void *arg) {
  while (1){
    lprintf("%s", (char *)arg);
  }
}

char* str[] = {"1", "2", "3"};
void createordelete(void *arg){
    while (1){
        task_t *tmp;
        for (int i=0;i<=2;++i){
            tmp = pmm->alloc(sizeof(task_t));
            kmt->create(tmp, "auto-gem", logging, str[i]);
        }
        _yield();
        kmt->teardown(tmp);
    }
}

static void os_init() {
  //Init spinlock
  #ifdef DEBUG_LOCK
  kmt->spin_init(&lock_debug, "debug");
  #endif
  kmt->spin_init(&lock_print, "print");
  kmt->spin_init(&lock_os, "os");
  //Init module
  pmm->init();
  kmt->init();
  //dev->init();
  //Test
  kmt->create(pmm->alloc(sizeof(task_t)), "first", createordelete, NULL);
}


static void os_run() {
  _intr_write(1);
  while (1) {
    _yield();
    Panic("SHOULD NOT REACH HERE");
  }
}

callback_t handlers[MAXCB];
int h_handlers;

static void os_on_irq(int seq, int event, handler_t handler) {
    handlers[h_handlers++] = (callback_t){handler, event, seq};
    int i = h_handlers-1;
    //Insert to make seq increase
    while (i&&handlers[i].seq<=handlers[i-1].seq){
        callback_t tmp = handlers[i-1];
        handlers[i-1] = handlers[i];
        handlers[i] = tmp;
    }
}

static _Context *os_trap(_Event ev, _Context *context) {
  //Special Check
  switch (ev.event){
    case _EVENT_ERROR:
        warning("%s\n", ev.msg);
        _halt(1);
    case _EVENT_IRQ_TIMER:
        //printf("T%c\n", "1234"[_cpu()]);
        return context;
  }
  Log("%d: %s", ev.event, ev.msg);
  //Call all valid handler
  _Context *ret = NULL;
  kmt->spin_lock(&lock_os);
  for (int i=0;i<h_handlers;++i){
      Assert(i==h_handlers-1||handlers[i].seq<=handlers[i+1].seq, "seq not increase");
      if (handlers[i].event == _EVENT_NULL || handlers[i].event == ev.event){
          _Context *next = handlers[i].handler(ev, context);
          if (next) ret = next;
      }
  }
  kmt->spin_unlock(&lock_os);

  return ret;
}

MODULE_DEF(os) {
  .init   = os_init,
  .run    = os_run,
  .trap   = os_trap,
  .on_irq = os_on_irq,
};
