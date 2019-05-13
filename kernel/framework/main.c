#include <kernel.h>
#include <klib.h>
#include <common.h>

sem_t empty, fill;
void producer(){
  while (1) {
    kmt->sem_wait(&empty);
    lprintf("(");
    kmt->sem_signal(&fill);
  }
}
void consumer(){
  while (1){
    lprintf("!");
    kmt->sem_wait(&fill);
    lprintf(")");
    kmt->sem_signal(&empty);
  }
}

int main() {
  _ioe_init();
  _cte_init(os->trap);

  // call sequential init code
  os->init();
  kmt->sem_init(&empty, "empty", 10);
  kmt->sem_init(&fill, "fill", 0);
  kmt->create(pmm->alloc(sizeof(task_t)), "producer", producer, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "consumer", consumer, NULL);

  _mpe_init(os->run); // all cores call os->run()
  return 1;
}
