#include <common.h>

static void os_init() {
  pmm->init();
}

static void os_run() {
  /*
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  */
  printf("os test\n");
  assert(pmm->alloc(4096)!=NULL);
  printf("first test\n");
  intptr_t* sg = 0;
  for(int i=0; i< 10; i++){
    sg = pmm->alloc(4096);
    pmm->free(sg);
    if(sg==NULL) {
      continue;
    } else {
      assert((((intptr_t)sg)&(1<<12))==((intptr_t)sg));
    }
  }
  printf("succeed!\n");
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
