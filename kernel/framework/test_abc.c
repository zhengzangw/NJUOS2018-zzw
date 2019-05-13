#include <kernel.h>
#include <klib.h>
#include <common.h>

void logging(void *arg) {
  while (1){
    lprintf("%s", (char *)arg);
    assertIF1();
  }
}

int main() {
  _ioe_init();
  _cte_init(os->trap);

  // call sequential init code
  os->init();
  kmt->create(pmm->alloc(sizeof(task_t)), "C", logging, "C");
  kmt->create(pmm->alloc(sizeof(task_t)), "A", logging, "A");
  kmt->create(pmm->alloc(sizeof(task_t)), "B", logging, "B");
  _mpe_init(os->run); // all cores call os->run()
  return 1;
}
