#include <kernel.h>
#include <klib.h>
#include <common.h>

void logging(void *arg) {
  while (1){
    lprintf("%s", (char *)arg);
    assertIF1();
  }
}

char* str[] = {"1", "2", "3", "4"};
void test(void *arg){
    while (1){
        task_t *tmp=NULL;
        for (int i=0;i<=3;++i){
            tmp = pmm->alloc(sizeof(task_t));
            if (kmt->create(tmp, "auto-gem", logging, str[i])){
                pmm->free(tmp);
                tmp = NULL;
            }
        }
        if (tmp) kmt->teardown(tmp);
    }
}

int main() {
  _ioe_init();
  _cte_init(os->trap);

  // call sequential init code
  os->init();
  kmt->create(pmm->alloc(sizeof(task_t)), "first", test, NULL);
  _mpe_init(os->run); // all cores call os->run()
  return 1;
}
