#include <common.h>
//#define DEBUG false
/**
 * @note get bits of the integer
 */
static int getb(size_t size) {
  int ret = 0;
  while(size > (1<<ret)){
    ret++;
  }
  return ret;
}

//=============== start define mutex lock ================
/*
typedef uintptr_t mutex_t;
#define MUTEX_INITIALIZER 0
void mutex_lock(mutex_t* locked);
void mutex_unlock(mutex_t* locked);
static uintptr_t atomic_xchg(volatile mutex_t* addr, uintptr_t newval) {
  uintptr_t result;
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
*/
static mutex_t big_lock = MUTEX_INITIALIZER;
//=============== end of definition =====================

//=============== start define page =====================
#define PAGE_SZ 8192
#define VOLUME 7*1024
typedef struct page {
  uint8_t data[VOLUME];
  //int sign;
  int cpu_id;
  int type;
  int count;
  mutex_t lock;
  //int max;
  struct page* next;
  uint32_t bitmap[128]; //512B

} page_t;
page_t* private_list[8][13][32];
uintptr_t heap_ptr;
//=============== end of definition =====================

static page_t* alloc_page(){
  page_t* ret = NULL;
  mutex_lock(&big_lock);
  if(heap_ptr > (uintptr_t)_heap.start+PAGE_SZ){
    heap_ptr -= PAGE_SZ;
    ret = (page_t*)heap_ptr;
  }
  mutex_unlock(&big_lock);
  return ret;
}

static void init_info(page_t* page, int cpu_id, int type){
  page->cpu_id = cpu_id;
  page->type = type;
  page->count = 0;
  page->next = NULL;
  for(int i=0; i<128; i++){
    page->bitmap[i] = 0;
  }
}

static bool full(page_t* page){
  if(page->count==256 || page->count == (VOLUME/(page->type))){
    return true;
  }else{
    return false;
  }
}

static void* kalloc(size_t size) {
  //if(DEBUG)printf("begin alloc\n");
  //assert(size > 0);
  //assert(size <= 4096);
  int ran = rand()%16;
  int bits = getb(size);
  page_t* cur = private_list[_cpu()][bits][ran];
  page_t* prev = cur;
  int cnt = 0;
  while(cur && full(cur)){
    prev = cur;
    cur = cur->next;
    cnt++;
    if(cnt > 1024)return NULL;
  }
  if(cur == NULL){
    cur = alloc_page();
    prev->next = cur;
    if(cur == NULL){
      return NULL;
    }
    init_info(cur, _cpu(), 1<<bits);
  }
  
  if(size == 4096){
    mutex_lock(&cur->lock);
    cur->bitmap[0] |= 1;
    cur->count += 1;
    mutex_unlock(&cur->lock);
    return (void*)((uintptr_t)cur);
  }
  
  int i=0;
  int j=0;
  uint32_t sign = 0;
  mutex_lock(&cur->lock);
  while(cur->bitmap[i]+1 == 0){
    i++;
  }
  sign = cur->bitmap[i];
  while((sign&(1<<j))){
    j++;
  }
  //assert(j>=0);
  //if(DEBUG)printf("change bitmap\n");
  //mutex_lock(&big_lock);
  //assert(j<32);
  cur->bitmap[i] |= (1<<j);
  cur->count += 1;
  mutex_unlock(&cur->lock);
  //printf("count %d\n i %d j %d\n", cur->count, i, j);
  putstr("finish kalloc\n");
  uintptr_t ret =  (((uintptr_t)cur)+((i*32+j)*(cur->type)));
  /*
  assert(ret > (uintptr_t)_heap.start);
  assert(ret < (uintptr_t)_heap.end);
  assert(ret % ((uintptr_t)(1<<bits)) == 0);
  */
  return (void*)ret;

}

static void kfree(void *ptr) {
  //if(DEBUG)printf("start free\n");
  uintptr_t mem_ptr = (uintptr_t)ptr;
  page_t* pg = (page_t*)(mem_ptr/8192*8192);
  //assert((mem_ptr - (uintptr_t)pg) < PAGE_SZ);
  uintptr_t num = (mem_ptr - (uintptr_t)pg)/(pg->type);
  uintptr_t i = num/32;
  uintptr_t j = num%32;
  mutex_lock(&pg->lock);
  pg->count -= 1;
  pg->bitmap[i] &= (~(1<<j));
  mutex_unlock(&pg->lock);
  //if(DEBUG)printf("end free\n");
}

static void pmm_init() {
  heap_ptr = (uintptr_t)_heap.end;
  int cpu_num = _ncpu();
  for(int i=0; i<cpu_num; i++){
    for(int j=0; j<13; j++){
      for(int k=0; k<16; k++){
        private_list[i][j][k] = alloc_page();
        init_info(private_list[i][j][k], i, (1<<j));
        //assert(private_list[i][j] != NULL);
      }
    }
  }
  //if(DEBUG) printf("init finished\n");
}

static void* kalloc_safe(size_t size){
  int i = _intr_read();
  _intr_write(0);
  void *ret = kalloc(size);
  if (i) _intr_write(1);

  return ret;
}

static void kfree_safe(void* ptr){
  int i = _intr_read();
  _intr_write(0);
  kfree(ptr);
  if (i) _intr_write(1);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  //.alloc = kalloc,
  .alloc = kalloc_safe,
  //.free  = kfree,
  .free = kfree_safe,
};
