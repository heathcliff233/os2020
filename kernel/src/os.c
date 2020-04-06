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
  void* sg1 = 0;
  void* sg2 = 0;
  void* sg3 = 0;
  void* sg4 = 0;
  void* sg5 = 0;
  void* sg6 = 0;
  int count = 0;
  //os_init();
  //void* tst[10] = {0,0,0,0,0,0,0,0,0,0};
  //tst = malloc(10*sizeof(intptr_t));
  
  for(int i=0; i<300; i++){
    /*
    for(int j=0; j<10; j++){
      tst[j] = pmm->alloc(4096);
      printf("finish alloc for %d time",j);
    }
    for(int k=0; k<10; k++){
      if(tst[k]==NULL){
        count++;
        continue;
      }
      pmm->free(tst[k]);
    }
    */
    sg1 = pmm->alloc(4096);
    sg2 = pmm->alloc(4096);
    sg3 = pmm->alloc(4096);
    sg4 = pmm->alloc(4096);
    sg5 = pmm->alloc(4096);
    sg6 = pmm->alloc(4096);

    //printf("finish alloc\n");
    pmm->free(sg1);
    pmm->free(sg2);
    pmm->free(sg3);
    pmm->free(sg4);
    pmm->free(sg5);
    pmm->free(sg6);
    //printf("now NO %d\n",i);
    /*
    sg = pmm->alloc(4096);
    if(sg==NULL) count++;
    sg = pmm->alloc(132);
    if(sg==NULL) count++;
    sg = pmm->alloc(532);
    if(sg==NULL) count++;
    */
    
    //printf("NO %d alloc succ\n",i);
    //if (((intptr_t)sg)/4096*4096!=(intptr_t)sg) printf("NO %d ptr %ld",i, (intptr_t)sg);
    //printf("to free\n");
    //pmm->free(sg);
    //printf("free success\n");
  }
  
  //sg = pmm->alloc(10);
  //printf("ptr %ld\n",sg);
  printf("total miss %d\n", count);
  printf("succeed!\n");
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
