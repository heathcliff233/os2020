//#include <kernel.h>
//#include <klib.h>

#include <common.h>

void producer(void *s){
  printf("%s\n", s);
  //return NULL;
}
static void create_threads() {
  kmt->create(pmm->alloc(sizeof(task_t)),"test-thread-1", producer, "xxx");
}

int main() {
  _ioe_init();
  _cte_init(os->trap);
  //_vme_init(pmm->alloc, pmm->free);
  os->init();
  //_putc('a');
  //_putc('\n');
  create_threads();
  _mpe_init(os->run);
  return 1;
}
