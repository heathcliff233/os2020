#include <common.h>
/*
typedef uintptr_t mutex_t;
#define MUTEX_INITIALIZER 0
void mutex_lock(mutex_t* locked);
void mutex_unlock(mutex_t* locked);
*/
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

/**
 * Modified from xv6
 */
/*
static inline uint32_t readeflags() {
  volatile uint32_t efl;
  asm volatile ("pushf; pop %0": "=r"(efl));
  return efl;
}
*/
int nintena[8] = {};
int ncli[8] = {};

void spinlock_self(){
  for (int i=0; i<8; i++) {
    ncli[i] = 0;
    nintena[i] = 1;
  }
}

void spinlock_init(spinlock_t* lk, const char* name) {
  lk->lock = MUTEX_INITIALIZER;
  lk->name = name;
  lk->owner = -1;
}

void spinlock_pushcli() {
  //int eflags = readeflags();
  uintptr_t it = _intr_read();
  _intr_write(0);
  if(ncli[_cpu()] == 0){
    //nintena[_cpu()] = eflags & FL_IF;
    nintena[_cpu()] = it;
  }
  ncli[_cpu()] += 1;
}

void spinlock_popcli() {
  //putstr("pop\n");
  int it = _intr_read();
  //if(readeflags()&FL_IF)
  if(it) {
    panic("popcli - interruptible");
  }
  ncli[_cpu()] -= 1;
  if(ncli[_cpu()] < 0) {
    panic("popcli");
  }
  if(ncli[_cpu()] == 0 && nintena[_cpu()]==1){
    _intr_write(1);
    putstr("open\n");
  }
}

int spinlock_holding(spinlock_t* lk){
  int r = 0;
  spinlock_pushcli();
  r = lk->lock && lk->owner == _cpu();
  spinlock_popcli();
  return r;
}

void spinlock_acquire(spinlock_t* lk){
  spinlock_pushcli();
  if(spinlock_holding(lk)) {
    putstr(lk->name);
    panic("acquire");
  }
  mutex_lock(&(lk->lock));
  __sync_synchronize();
  lk->owner = _cpu();
}

void spinlock_release(spinlock_t* lk){
  if(!spinlock_holding(lk)) panic("release");
  lk->owner = -1;
  __sync_synchronize();
  mutex_unlock(&(lk->lock));
  spinlock_popcli();
  putstr("lk name ");
  putstr(lk->name);
  return;
}
