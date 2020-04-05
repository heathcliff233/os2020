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
  //printf("os test\n");
  //assert(pmm->alloc(4096)!=NULL);
  //printf("first test\n");
  void* sg = 0;
  //os_init();
  
  for(int i=0; i< 100; i++){
    sg = pmm->alloc(4096);
    //printf("to free\n");
    pmm->free(sg);
    printf("free success\n");
  }
  
  //sg = pmm->alloc(10);
  //printf("ptr %ld\n",sg);
  printf("succeed!\n");
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
