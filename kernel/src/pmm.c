#include <common.h>
//#include <klib.h>

#define PAGE_SIZE 8192
#define HDR_SIZE 8

#define align(_A,_B) (_A=((_A-1)/_B+1)*_B)

typedef intptr_t mutex_t;
#define MUTEX_INITIALIZER 0
void mutex_lock(mutex_t* locked);
void mutex_unlock(mutex_t* locked);

static intptr_t atomic_xchg(volatile mutex_t* addr,intptr_t newval) {
  intptr_t result;
  asm volatile ("lock xchg %0, %1":
    			"+m"(*addr), "=a"(result) : 
    			"1"(newval) :
    			"cc");
  return result;
}

void pthread_mutex_lock(mutex_t* lk) {
  while (atomic_xchg(&lk, 1));
}
void pthread_mutex_unlock(mutex_t* lk) {
  atomic_xchg(&lk, 0);
}

static mutex_t big_lock = MUTEX_INITIALIZER;

typedef struct mem_block {
	size_t size;
	struct mem_block* next; 
} mem_head;

typedef union page {
  struct {
    mutex_t lock;
    size_t size;
    union page* next;
    union page* prev;
    mem_head* list;
  }; 
  uint8_t header[HDR_SIZE], data[PAGE_SIZE - HDR_SIZE];
} __attribute__((packed)) page_t;

typedef struct Header {
	struct Header* next;
	page_t* pg;
} header_t;

page_t* private_list[8]={NULL};

uint16_t pages[1<<15]={};

static intptr_t* alloc_new_page() {
	return NULL;
}

static void* alloc_small() {
	return NULL;
}

static void* alloc_big() {
	return NULL;
}

static void *kalloc(size_t size) {
  return NULL;
}

static void kfree(void *ptr) {
}

static void pmm_init() {
  uintptr_t pmstart = (uintptr_t)_heap.start;
  uintptr_t pmsize = ((uintptr_t)_heap.end - align(pmstart, PAGE_SIZE));
  //printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, _heap.start, _heap.end);
  int cpu_cnt = _ncpu();
  for(int i=0; i<cpu_cnt; i++) {
    private_list[i] = alloc_new_page();
    private_list[i]->lock = 0;
  }
  for(int i=0; i<pmsize/PAGE_SIZE && i; i++) {}
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
