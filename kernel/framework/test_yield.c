#include <kernel.h>
#include <klib.h>
#include <common.h>

void non(){
  while (1){
    _yield();
  }
}

int main() {
  _ioe_init();
  _cte_init(os->trap);

  // call sequential init code
  os->init();
  for (int i=0;i<10;++i){
	kmt->create(pmm->alloc(sizeof(task_t)), "non", non, NULL);
  }

  _mpe_init(os->run); // all cores call os->run()
  return 1;
}
