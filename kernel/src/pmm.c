#include <common.h>
//#include <klib.h>

#define PAGE_SIZE 8192*16+512
#define HDR_SIZE 32
#define SG_SIZE 24

#define align(_A,_B) (((_A-1)/_B+1)*_B)

typedef intptr_t mutex_t;
#define MUTEX_INITIALIZER 0
void mutex_lock(mutex_t* locked);
void mutex_unlock(mutex_t* locked);
int num_avai_page;

static int getb(size_t size) {
  int ret = 0;
  while(size > (1<<ret)){
    ret++;
  }
  return (1<<ret);
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
  intptr_t hd_sp;
  //bool available;
	size_t size;
  //struct mem_block* prev;
	struct mem_block* next; 
} mem_head;

typedef union page {
  struct {
    //mutex_t lock;
    //size_t size;
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
  page_t* ret = NULL;
  
  
  //assert(free_list->next != NULL);
  //! mutex_lock(&big_lock);
  if(num_avai_page > 8) {
  //if(free_list->next != NULL){
    ret = free_list;
    free_list = free_list->next;
    //printf("%ld\n", free_list);
    free_list->prev = free_list;
    //printf("stop\n");
  //}
    num_avai_page--;
  }
  //! mutex_unlock(&big_lock);
  if(ret==NULL) return ret;
  //! mutex_lock(&big_lock);
  ret->chart = (mem_head*)((intptr_t)ret + HDR_SIZE); 
  ret->chart->next = NULL;
  ret->chart->size = SG_SIZE;
  ret->count = 0;
  //! mutex_unlock(&big_lock);
  //printf("finish\n");
  
	return ret;
}
/*
static void* alloc_small(size_t size) {
  //! mutex_lock(&big_lock);
  int cpu_id = _cpu();
  page_t* cur_page = private_list[cpu_id];
  assert(cur_page->count >= 0);
  mem_head* tmp = cur_page->chart;
  //cur_page->chart->next->sp = align((tmp->sp+tmp->size), getb(size));
  cur_page->chart->next = (mem_head*)align(((intptr_t)tmp+tmp->size), getb(size));
  cur_page->chart = cur_page->chart->next;
  cur_page->chart->size = size;
  cur_page->chart->hd_sp = (intptr_t)cur_page;
  //printf("page head %ld\n",(intptr_t)cur_page);
  
  cur_page->count += 1;
  //! mutex_unlock(&big_lock);
  //printf("alloc num %d\n",cur_page->count);
  
  //printf("return ptr %ld\n",(intptr_t)cur_page->chart);
	return (void*)((intptr_t)(cur_page->chart));
}
*/
static void *kalloc(size_t size) {
  //printf("begin alloc \n");
  if(size == 0){
    return NULL;
  } else {
    size_t tot = size + SG_SIZE;
    //size += SG_SIZE;
    int cpu_id = _cpu();
    page_t* cur = (page_t*)private_list[cpu_id];
    size_t used = align((intptr_t)(cur->chart)+cur->chart->size,getb(size))+tot-(intptr_t)cur;
    /*
    printf("align %d",getb(size));
    printf("current %ld",align((intptr_t)(cur->chart),getb(tot)));
    printf("used %ld\n",used);
    printf("page ptr %ld\n",(intptr_t)cur);
    printf("memblock ptr %ld\n",(intptr_t)(cur->chart));
    */
   mutex_lock(&big_lock);
    if(used > PAGE_SIZE) {
      //printf("lock\n");
      
      page_t* tmp = alloc_new_page();
      
      //printf("unlock\n");
      if(tmp==NULL) {
        mutex_unlock(&big_lock);
        return NULL;
      } else {
        private_list[cpu_id]->next = tmp;
        tmp->prev = private_list[cpu_id];
        private_list[cpu_id] = private_list[cpu_id]->next;
        private_list[cpu_id]->chart = (mem_head*)((intptr_t)tmp + HDR_SIZE);
        /*
        private_list[cpu_id]->chart->next = NULL;
        private_list[cpu_id]->chart->size = SG_SIZE;
        private_list[cpu_id]->count = 0;
        */
      }
    }

    //printf("begin small alloc \n");
    //! return alloc_small(size);
    
    page_t* cur_page = private_list[cpu_id];
    mem_head* tmp = cur_page->chart;
    cur_page->chart->next = (mem_head*)align((intptr_t)tmp+tmp->size,getb(size));
    cur_page->chart = cur_page->chart->next;
    cur_page->chart->hd_sp = (intptr_t)cur_page;
    cur_page->chart->size = size;
    cur_page->count += 1;
    mutex_unlock(&big_lock);
    return (void*)((intptr_t)(cur_page->chart));
    
    
  }
  return NULL;
}

int buf = 0;
static void free_page(page_t* pg){
	assert(pg->count == 0);
	if(pg->prev == pg) return;
	pg->prev->next = pg->next;
	if(pg->next != NULL) pg->next->prev = pg->prev;
	pg->prev = pg;
	pg->next = free_list;
	free_list->prev = pg;
	free_list = free_list->next;
}
static void kfree(void* ptr) {
  
  
  //printf("free\n");
  //printf("feed %ld\n",(intptr_t)ptr);
  //page_t* hd = (page_t*)(((uintptr_t)ptr-(uintptr_t)_heap.start)/PAGE_SIZE*PAGE_SIZE+(uintptr_t)_heap.start);
  
  
  mutex_lock(&big_lock);
  page_t* hd = (page_t*)(((mem_head*)ptr)->hd_sp);
  assert(hd->count > 0);
  hd->count -= 1;
  buf++;
  mutex_unlock(&big_lock);

  if(buf > 204){
  	buf = 0;
  	page_t* iter = private_list[_cpu()];
  	while(iter){
  		mutex_lock(&big_lock);
  		if(iter->count == 0){
  			free_page(iter);
  		}
  		mutex_unlock(&big_lock);
  	}
  	printf("good free\n");
  }
  //printf("remaining count %d\n",hd->count);
  //printf("free count %d\n",hd->count);
  /*
  if(hd->count == 0) {
    
    //if(hd->prev != hd){
      //mutex_lock(&big_lock);
      assert(hd->prev != NULL);
      hd->prev->next = hd->next;
      //printf("free page\n");
      if(hd->next != NULL) hd->next->prev = hd->prev;
      //page_t* cp_free_list  = free_list;
      //hd->next = cp_free_list;
      hd->next = free_list;
      free_list->prev = hd;
      hd->prev = hd;
      free_list = hd;
      //mutex_unlock(&big_lock);
      num_avai_page++;
      printf("num page available %d\n",num_avai_page);
    //}
  }
  */
  //mutex_unlock(&big_lock);
  //printf("finish free\n");
  
}

static void pmm_init() {
  num_avai_page = 0;
  //intptr_t pmstart = (intptr_t)_heap.start;
  //intptr_t pmsize = ((intptr_t)_heap.end - align(pmstart, PAGE_SIZE));
  free_list = (page_t*)(align(((intptr_t)_heap.start),4096));
  page_t* cp = free_list;
  free_list->prev = free_list;
  //printf("start point %ld\n",free_list);
  page_t* st = NULL;
  while((intptr_t)free_list < (intptr_t)_heap.end - PAGE_SIZE) {
  	num_avai_page++;
    st = free_list;
  	//free_list->lock = 0;
  	
  	free_list->next = (page_t*)((intptr_t)free_list + PAGE_SIZE);
  	free_list = free_list->next;
    free_list->prev = st;
    //printf("pgpoint %ld\n",free_list);
  }
  free_list = cp;
  int cpu_cnt = _ncpu();
  //printf("alloc for cpu\n");
  for(int i=0; i<cpu_cnt; i++) {
    private_list[i] = alloc_new_page();
    private_list[i]->prev = private_list[i];
    //private_list[i]->lock = 0;
    //printf("cpuid %d\n",i);
  }
  //printf("init finished\n");
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
