#include <kernel.h>
#include <klib.h>

int main() {
  _ioe_init();
  _halt(1);
  _cte_init(os->trap);

  // call sequential init code
  os->init();
  _mpe_init(os->run); // all cores call os->run()
  return 1;
}
