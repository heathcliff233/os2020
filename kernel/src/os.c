#include <common.h>

static void os_init() {
  pmm->init();
}

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  assert(pmm->alloc(4096)!=NULL);
  intptr_t* sg = 0;
  for(int i=0; i< 100; i++){
    sg = pmm->alloc(4096);
    if(sg==NULL) {
      continue;
    } else {
      assert((((intptr_t)sg)&(1<<12))==((intptr_t)sg));
    }
  }
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
