#include <common.h>
#include <klib.h>

lock_t lock_test;
static void os_init() {
  pmm->init();
  init(&lock_test);
}

static void hello() {
  lock(&lock_test);
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++){
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
  unlock(&lock_test);
}

static void test() {
  lock(&lock_test);
  int len = rand()%1024;
  char *str = pmm->alloc(len);
  for (int i=0;i<len;++i){
    str[i] = 'A'+i%24;
  }
  
  printf("%s\n", str);
  unlock(&lock_test);
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
