#include <common.h>
//#include <klib.h>

#define PAGE_SIZE 8192
#define HDR_SIZE 40
#define SG_SIZE 24

#define align(_A,_B) (((_A-1)/_B+1)*_B)

typedef intptr_t mutex_t;
#define MUTEX_INITIALIZER 0
void mutex_lock(mutex_t* locked);
void mutex_unlock(mutex_t* locked);

static int getb(size_t size) {
  int ret = 0;
  while(size < (1<<ret)){
    ret++;
  }
  return ret;
}

static intptr_t atomic_xchg(volatile mutex_t* addr,intptr_t newval) {
  intptr_t result;
  asm volatile ("lock xchg %0, %1":
    			"+m"(*addr), "=a"(result) : 
    			"1"(newval) :
    			"cc");
  return result;
}

void mutex_lock(mutex_t* lk) {
  while (atomic_xchg(lk, 1));
}
void mutex_unlock(mutex_t* lk) {
  atomic_xchg(lk, 0);
}

static mutex_t big_lock = MUTEX_INITIALIZER;

typedef struct mem_block {
  intptr_t sp;
  //bool available;
	size_t size;
  //struct mem_block* prev;
	struct mem_block* next; 
} mem_head;

typedef union page {
  struct {
    //mutex_t lock;
    size_t size;
    intptr_t count;
    union page* next;
    union page* prev;
    mem_head* chart;
  }; 
  uint8_t header[HDR_SIZE], data[PAGE_SIZE - HDR_SIZE];
} __attribute__((packed)) page_t;

page_t* private_list[8]={NULL};
page_t* free_list = NULL;

static page_t* alloc_new_page() {
  //printf("allocing new page\n");
  mutex_lock(&big_lock);
  page_t* ret = free_list;
  free_list = free_list->next;
  //printf("%ld\n", free_list);
  free_list->prev = NULL;
  //printf("stop\n");
  mutex_unlock(&big_lock);
  //printf("finish\n");
	return ret;
}

static void* alloc_small(size_t size) {
  int cpu_id = _cpu();
  //mutex_lock(&private_list[cpu_id]->lock);
  page_t* cur_page = private_list[cpu_id];
  mem_head* tmp = cur_page->chart;
  cur_page->chart->next->sp = align((tmp->sp+tmp->size), getb(size));
  cur_page->chart->next->size = size;
  //cur_page->chart->next->prev = cur_page->chart;
  //mutex_unlock(&private_list[cpu_id]->lock);
  cur_page->count += 1;
	return (void*)cur_page->chart->next->sp;
}


static void *kalloc(size_t size) {
  //printf("begin alloc \n");
  if(size==0){
    return NULL;
  } else {
    size += SG_SIZE;
    size = size>4096 ? 4096:size;
    int cpu_id = _cpu();
    if(size > private_list[cpu_id]->size) {
      //mutex_lock(&big_lock);
      page_t* tmp = alloc_new_page();
      //mutex_unlock(&big_lock);
      printf("ublock\n");
      if(tmp==NULL) {
        return NULL;
      } else {
        private_list[cpu_id]->next = tmp;
        private_list[cpu_id] = private_list[cpu_id]->next;
        private_list[cpu_id]->chart = (mem_head*)((intptr_t)tmp + HDR_SIZE);
        private_list[cpu_id]->chart->next = NULL;
        private_list[cpu_id]->chart->size = SG_SIZE;
        private_list[cpu_id]->count = 0;
      }
    }
    printf("begin small alloc \n");
    return alloc_small(size);
  }
  return NULL;
}

static void kfree(void *ptr) {
  page_t* hd = (page_t*)((uintptr_t)ptr & (1<<13));
  hd->count -= 1;
  if(hd->count < 100) {
    if(hd->prev == hd) return;
    mutex_lock(&big_lock);
    hd->prev->next = hd->next;
    page_t* cp_free_list  = free_list;
    hd->next = cp_free_list->next;
    cp_free_list->next = hd;
    mutex_unlock(&big_lock);
  }
}

static void pmm_init() {
  //intptr_t pmstart = (intptr_t)_heap.start;
  //intptr_t pmsize = ((intptr_t)_heap.end - align(pmstart, PAGE_SIZE));
  free_list = _heap.start;
  printf("start point %ld\n",free_list);
  page_t* st = NULL;
  while((intptr_t)free_list < (intptr_t)_heap.end - PAGE_SIZE) {
  	st = free_list;
  	//free_list->lock = 0;
  	free_list->prev = st;
  	free_list->next = (page_t*)((intptr_t)free_list + PAGE_SIZE);
  	free_list = free_list->next;
    //printf("pgpoint %ld\n",free_list);
  }
  free_list = _heap.start;
  int cpu_cnt = _ncpu();
  //printf("alloc for cpu\n");
  for(int i=0; i<cpu_cnt; i++) {
    private_list[i] = alloc_new_page();
    private_list[i]->prev = private_list[i];
    //private_list[i]->lock = 0;
    printf("cpuid %d\n",i);
  }
  printf("init finished\n");
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
