#include <kernel.h>
#include <klib.h>

int main() {
  _ioe_init();
  _cte_init(os->trap);
  _halt(1);

  // call sequential init code
  os->init();
  _mpe_init(os->run); // all cores call os->run()
  return 1;
}
