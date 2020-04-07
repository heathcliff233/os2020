#include <common.h>
//#include <klib.h>

#define PAGE_SIZE 8192+512
#define HDR_SIZE 32
//#define SG_SIZE 24

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
  ret = ret>3 ? ret:3;
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
  struct mem_block* next; 
  intptr_t hd_sp;
  //bool available;
	size_t dummy;
  //struct mem_block* prev;
  uint8_t start;
} mem_head;
#define SG_SIZE (sizeof(mem_head)-sizeof(size_t))
typedef union page {
  struct {
    // mutex_t lock;
    size_t size;
    // intptr_t count;
    union page* next;
    union page* prev;
    mem_head* chart;
  }; 
  uint8_t header[HDR_SIZE], data[PAGE_SIZE - HDR_SIZE];
} __attribute__((packed)) page_t;

page_t* private_list[8]={NULL};
page_t* free_list = NULL;
page_t* alloc_list = NULL;

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
  ret->size = PAGE_SIZE - HDR_SIZE - SG_SIZE;
  ret->next = alloc_list->next;
  ret->prev = alloc_list;
  if(alloc_list->next != NULL) alloc_list->next->prev = ret;
  alloc_list->next = ret;
  ret->chart = (mem_head*)((intptr_t)ret + HDR_SIZE); 
  ret->chart->next = NULL;
  ret->chart->hd_sp = 0;
  //ret->count = 0;
  //! mutex_unlock(&big_lock);
  //printf("finish\n");
  
	return ret;
}
/*
static void* alloc_small(size_t size) {
  //! mutex_lock(&big_lock);
  int cpu_id = _cpu();
  page_t* cur_page = private_list[cpu_id];
  //assert(cur_page->count >= 0);
  mem_head* tmp = cur_page->chart;
  //cur_page->chart->next->sp = align((tmp->sp+tmp->size), getb(size));
  cur_page->chart->next = (mem_head*)align(((intptr_t)tmp+tmp->size), getb(size));
  cur_page->chart = cur_page->chart->next;
  cur_page->chart->size = size;
  //cur_page->chart->hd_sp = (intptr_t)cur_page;
  //printf("page head %ld\n",(intptr_t)cur_page);
  
  //! cur_page->count += 1;
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
    //size_t tot = size + SG_SIZE;
    //size += SG_SIZE;
    size = align(size,8);
    int cpu_id = _cpu();
    //page_t* cur = (page_t*)private_list[cpu_id];
    mem_head* cur = private_list[cpu_id]->chart;
    //size_t used = align((intptr_t)(cur->chart)+cur->chart->size,getb(size))+tot-(intptr_t)cur;
    intptr_t sp = align((intptr_t)(intptr_t)(&cur->start),getb(size));
    size_t blank = sp - (intptr_t)(&cur->start);
    size_t sz = blank + sp;
    /*
    printf("align %d",getb(size));
    printf("current %ld",align((intptr_t)(cur->chart),getb(tot)));
    printf("used %ld\n",used);
    printf("page ptr %ld\n",(intptr_t)cur);
    printf("memblock ptr %ld\n",(intptr_t)(cur->chart));
    */
   
    //if(used > PAGE_SIZE) {
    if(sz+SG_SIZE) {
      //printf("lock\n");
      mutex_lock(&big_lock);
      page_t* tmp = alloc_new_page();
      mutex_unlock(&big_lock);
      //printf("unlock\n");
      if(tmp==NULL) {
        return NULL;
      } else {
        private_list[cpu_id]->next = tmp;
        cur = private_list[cpu_id]->chart;
        sp = align((intptr_t)(&cur->start),getb(size));
        blank = sp - (intptr_t)(&cur->start);
        sz = blank + size;
        //tmp->prev = private_list[cpu_id];
        //private_list[cpu_id] = private_list[cpu_id]->next;
        //private_list[cpu_id]->chart = (mem_head*)((intptr_t)tmp + HDR_SIZE);
        /*
        private_list[cpu_id]->chart->next = NULL;
        private_list[cpu_id]->chart->size = SG_SIZE;
        private_list[cpu_id]->count = 0;
        */
      }
    }

    //printf("begin small alloc \n");
    private_list[cpu_id]->chart = (mem_head*)(sp+size);
    private_list[cpu_id]->chart->next = NULL;
    private_list[cpu_id]->chart->hd_sp = 0;
    private_list[cpu_id]->size = private_list[cpu_id]->size - SG_SIZE - sz;
    cur->hd_sp = sp;
    memcpy((void*)(sp-sizeof(size_t)), &blank, sizeof(size_t));
    cur->next = private_list[cpu_id]->chart;
    return (void*)cur->hd_sp;
    //! return alloc_small(size);
    /*
    page_t* cur_page = private_list[cpu_id];
    mem_head* tmp = cur_page->chart;
    cur_page->chart->next = (mem_head*)align((intptr_t)tmp+tmp->size,getb(size));
    cur_page->chart = cur_page->chart->next;
    cur_page->chart->hd_sp = (intptr_t)cur_page;
    cur_page->chart->size = size;
    cur_page->count += 1;
    */
    //mutex_unlock(&big_lock);
    //return (void*)((intptr_t)(cur_page->chart));
    
    
  }
  return NULL;
}
/*
static void kfree(void *ptr) {
  
  
  //printf("free\n");
  //printf("feed %ld\n",(intptr_t)ptr);
  //page_t* hd = (page_t*)(((uintptr_t)ptr-(uintptr_t)_heap.start)/PAGE_SIZE*PAGE_SIZE+(uintptr_t)_heap.start);
  
  
  mutex_lock(&big_lock);
  page_t* hd = (page_t*)(((mem_head*)ptr)->hd_sp);
  assert(hd->count > 0);
  hd->count -= 1;
  //printf("remaining count %d\n",hd->count);
  //printf("free count %d\n",hd->count);
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
  mutex_unlock(&big_lock);
  //printf("finish free\n");

}
*/
static void free_page(page_t* pg){
  pg->prev->next = pg->next;
  if(pg->next != NULL){
    pg->next->prev = pg->prev;
  }
  pg->next = free_list;
  pg->prev = pg;
  free_list->prev = pg;
  free_list = pg;
  num_avai_page++;
}

int buf = 0;
static void kfree(void *ptr) {
  intptr_t sp = (intptr_t)ptr;
  size_t blank = *((size_t*)(sp - sizeof(size_t)));
  mem_head* cur = (mem_head*)(sp - blank - SG_SIZE);
  mutex_lock(&big_lock);
  buf++;
  if(cur->hd_sp == sp) cur->hd_sp = 0;
  mutex_unlock(&big_lock);
  if(buf<1000){
    return;
  } else {
    buf = 0;
    mutex_lock(&big_lock);
    page_t* pg = alloc_list->next;
    page_t* target = NULL;
    bool freepg = true;
    mem_head* p = NULL;
    while(cur){
      freepg = true;
      p = (mem_head*)((intptr_t)pg + HDR_SIZE);
      while(p) {
        if(p->hd_sp != 0) {
          freepg = 0;
          break;
        }
        p = p->next;
      }
      if(freepg){
        target = pg;
        pg = pg->prev;
        free_page(target);
      }
      pg = pg->next;

    }
    mutex_unlock(&big_lock);
  }
}

static void pmm_init() {
  num_avai_page = 0;
  //intptr_t pmstart = (intptr_t)_heap.start;
  //intptr_t pmsize = ((intptr_t)_heap.end - align(pmstart, PAGE_SIZE));
  free_list = (page_t*)(align(((intptr_t)_heap.start),4096));
  page_t* cp = free_list;
  //free_list->prev = free_list;
  //printf("start point %ld\n",free_list);
  page_t* st = NULL;
  while((intptr_t)free_list < (intptr_t)_heap.end - PAGE_SIZE) {
  	num_avai_page++;
    st = free_list;
  	//free_list->lock = 0;
  	free_list->chart = NULL;
  	free_list->next = (page_t*)((intptr_t)free_list + PAGE_SIZE);
  	free_list = free_list->next;
    free_list->prev = st;
    //printf("pgpoint %ld\n",free_list);
  }
  free_list = cp;
  free_list = free_list->next;
  free_list->prev = free_list;
  int cpu_cnt = _ncpu();
  //printf("alloc for cpu\n");
  alloc_list = cp;
  alloc_list->next = NULL;
  alloc_list->size = 0;
  alloc_list->prev = NULL;
  alloc_list->chart = NULL;
  for(int i=0; i<cpu_cnt; i++) {
    private_list[i] = alloc_new_page();
    //private_list[i]->prev = private_list[i];
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
