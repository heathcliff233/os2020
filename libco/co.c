/**
 * @author Bright Hong
 * @date 2020.3.3
 * @brief This is a shared library for coroutine.
 * @remarks 
 */

#include "co.h"
#include <stdlib.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

#if defined (__i386__)
  #define SP "%%esp"
#elif defined (__x86_64__)
  #define SP "%%rsp"
#endif

#define setSP(sp) \
  asm volatile("mov " "%0," SP : : "g"(sp))
#define getSP(sp) \
  asm volatile("mov " SP ", %0" : "=g"(sp) : )
#define movto(dest, stack) \
  *(uintptr_t*)(stack + (((void*)&dest)-stack_backup))=(uintptr_t)dest;
#define MAX_CO 128
#define DEBUG true
#define SZ_STACK 4096

struct co {
  union {
    struct {
      jmp_buf tar_buf;
      uint8_t stat;
    };
    uint8_t stack[SZ_STACK];
  };
}routines[MAX_CO],*current;

static int pool[MAX_CO];
static int pool_id = 0;
static jmp_buf ret_buf;
void* stack_backup = NULL;

static struct co* new_co() {
  pool_id--;
  //routines[pool[pool_id]].stat = CO_NEW;
  //return &routines[pool[pool_id]];
  routines[pool_id].stat = CO_NEW;
  return &routines[pool_id];
}

void co_init() {
  int i;
  pool_id = MAX_CO;
  for(i=0; i<MAX_CO; i++){
    //pool[i] = i;
    routines[i].stat = 0;
  }
}

struct co* co_start(const char *name, void (*func)(void *), void *arg) {
  if(pool_id==0){
    co_init();
  }
  getSP(stack_backup);
  current = new_co();
  uint8_t* stack_top = current->stack + SZ_STACK - SZ_STACK/4;
  movto(name,stack_top);
  movto(func,stack_top);
  movto(arg,stack_top);

  setSP(stack_top);
  if(!setjmp(current->tar_buf)) {
    setSP(stack_backup);
//    return current;
  } else {
    func(arg);
    current->stat = CO_DEAD;
    //pool[pool_id++] = current-routines;
    longjmp(ret_buf,1);
  }
  //return NULL;
  return current;
}

static void co_jmp(struct co* co){
  current = co;
  longjmp(co->tar_buf,1);
}

void co_yield() {
  if(!setjmp(current->tar_buf)){
    int next_co;
    do{
      next_co = rand()%MAX_CO;
    } while(!(routines[next_co].stat==CO_NEW));
    co_jmp(routines+next_co);
  }
}

void co_wait(struct co* co) {
  if(!setjmp(ret_buf)) {
    while(co->stat == CO_NEW){
      co_jmp(co);
    }
  }
  co->stat = 0;
}
