#include <common.h>
#define DEBUG false
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
typedef uintptr_t mutex_t;
#define MUTEX_INITIALIZER 0
void mutex_lock(mutex_t* locked);
void mutex_unlock(mutex_t* locked);
static uintptr_t atomic_xchg(volatile mutex_t* addr, uintptr_t newval) {
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
//=============== end of definition =====================

//=============== start define page =====================
#define PAGE_SZ 8192
#define VOLUME 7*1024
typedef struct page {
  uint8_t data[VOLUME];
  int sign;
  int cpu_id;
  int type;
  int count;
  //int max;
  struct page* next;
  uint32_t bitmap[128]; //512B

} page_t;
page_t* private_list[8][13];
uintptr_t heap_ptr;
//=============== end of definition =====================

static page_t* alloc_page(){
  page_t* ret = NULL;
  mutex_lock(&big_lock);
  if(heap_ptr > (intptr_t)_heap.start+PAGE_SZ){
    heap_ptr -= PAGE_SZ;
    ret = (page_t*)heap_ptr;
  }
  mutex_unlock(&big_lock);
  return ret;
}

static void init_info(page_t* page, int cpu_id, int type, int sign){
  page->cpu_id = cpu_id;
  page->sign = sign;
  page->type = type;
  page->count = 0;
  page->next = NULL;
  for(int i=0; i<128; i++){
    page->bitmap[i] = 0;
  }
}

static bool full(page_t* page){
  if(page->count == (VOLUME/(page->type))){
    return true;
  }else{
    return false;
  }
}

static void* kalloc(size_t size) {
  if(DEBUG)printf("begin alloc\n");
  mutex_lock(&big_lock);
  assert(size > 0);
  assert(size <= 4096);
  int bits = getb(size);
  page_t* cur = private_list[_cpu()][bits];
  page_t* prev = cur;
  while(cur){
    prev = cur;
    if(full(cur)){
      cur = cur->next;
    }else{
      break;
    }
  }
  if(cur == NULL){
    cur = alloc_page();
    prev->next = cur;
    if(cur == NULL){
      return NULL;
    }
    init_info(cur, _cpu(), 1<<bits, 1);
  }
  int i=0;
  while(cur->bitmap[i]+1 == 0){
    i++;
  }
  int j=31;
  while((cur->bitmap[i]&(1<<j))){
    j--;
  }
  assert(j>=0);
  //if(DEBUG)printf("change bitmap\n");
  //mutex_lock(&big_lock);
  cur->bitmap[i] |= (1<<j);
  cur->count += 1;
  mutex_unlock(&big_lock);
  //if(DEBUG)printf("finish kalloc\n");
  return (void*)(((uintptr_t)cur)+((i*32+j)<<bits));

}

static void kfree(void *ptr) {

}

static void pmm_init() {
  heap_ptr = (uintptr_t)_heap.end;
  int cpu_num = _ncpu();
  for(int i=0; i<cpu_num; i++){
    for(int j=1; j<13; j++){
      private_list[i][j] = alloc_page();
      init_info(private_list[i][j], i, (1<<j), 0);
      assert(private_list[i][j] != NULL);
    }
  }
  if(DEBUG) printf("init finished\n");
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
