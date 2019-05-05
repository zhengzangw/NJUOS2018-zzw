#include <common.h>

static void hello() {
  kmt->spin_lock(&lock_test);
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++){
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
  kmt->spin_unlock(&lock_test);
}

/*
static void mem_test() {
    int base = 0x10000;
    while (1){
        int len = base * (rand()%5)+(rand()%0x100);
        char *str = pmm->alloc(len);
        if (str){
            for (int i =0;i<len;++i){
                str[i] = 'A'+i%24;
            }
            if (rand()%3 ==0)
                pmm->free(str);
        }
    }
}
*/
