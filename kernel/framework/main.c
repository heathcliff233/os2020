#include <kernel.h>
#include <klib.h>

int main() {
  _ioe_init();
  _cte_init(os->trap);
  _vme_init(pmm->alloc, pmm->free);
  os->init();
  printf("os init finished\n");
  _mpe_init(os->run);
  return 1;
}
