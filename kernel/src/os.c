#include <common.h>
#include <klib.h>

#define Logcpu() printf("cpu #%c:\n", "12345678"[_cpu()]);

lock_t lock_test;
static void os_init() {
  pmm->init();
  init(&lock_test);
}

/*
static void hello() {
  lock(&lock_test);
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++){
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
  unlock(&lock_test);
}
*/

static void test() {
  int base = 0x10000;
  while (1){
    int len = base * (rand()%5)+(rand()%0x100);
    char *str = pmm->alloc(len);
    if (str){
      for (int i=0;i<len;++i){
        str[i] = 'A'+i%24;
      }
      if (rand()%3==0)
        pmm->free(str);
    } else {
      base = 0;
    }
  }
}

static void os_run() {
  //hello();
  test();
  _intr_write(1);
  while (1) {
    _yield();
  }
}

static _Context *os_trap(_Event ev, _Context *context) {
  return context;
}

static void os_on_irq(int seq, int event, handler_t handler) {
}

MODULE_DEF(os) {
  .init   = os_init,
  .run    = os_run,
  .trap   = os_trap,
  .on_irq = os_on_irq,
};
