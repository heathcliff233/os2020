#include "klib.h"
#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static intptr_t atomic_xchg(volatile pthread_mutex_t* addr,intptr_t newval) {
  intptr_t result;
  asm volatile ("lock xchg %0, %1":
    			"+m"(*addr), "=a"(result) : 
    			"1"(newval) :
    			"cc");
  return result;
}

void pthread_mutex_lock(pthread_mutex_t* lk) {
  while (atomic_xchg(&lk->locked, 1));
}
void pthread_mutex_unlock(pthread_mutex_t* lk) {
  atomic_xchg(&lk->locked, 0);
}
#endif