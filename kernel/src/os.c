#include <common.h>
#include <klib.h>
#include <lock.h>

lock_t lock_hello,lock_test;
static void os_init() {
  pmm->init();
  init(&lock_hello);
  init(&lock_test);
}

static void hello() {
  lock(&lock_hello);
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++){
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
  unlock(&lock_hello);
}

static void test() {
  char *str = pmm->alloc(rand()%1024);
  for (int i=0;i<strlen(str);++i){
    str[i] = 'A'+i%24;
  }
  
  lprintf(&lock_test, "%s\n", str);
}

static void os_run() {
  hello();
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
