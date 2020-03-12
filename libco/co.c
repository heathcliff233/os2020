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
#include <time.h>


#define DEBUG false
#define SZ_STACK 16*4096
//#define NR_CO 16

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

#if defined (__i386__)
  #define MASK 3
#elif defined (__x86_64__)
  #define MASK 7
#endif

static inline void stackEX(void *sp, void *entry, uintptr_t arg){
  asm volatile (
    #if defined (__x86_64__)
      "movq %0, %%rsp; movq %2, %%rdi; jmp *%1" : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
    #else
      "movl %0, %%esp; movl %2, 4(%0); jmp *%1" : : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
    #endif
  );
}

struct co {
  enum co_status state;
  const char* name;
  void (*func)(void *);
  void* arg;
  struct co* next;
  jmp_buf buf;
  char stack[SZ_STACK];
};

struct co* head = NULL;
struct co* current = NULL;



int tot=0;

__attribute__((constructor))static void Initiate()
{
  srand(time(NULL));
  head = malloc(sizeof(struct co));
  head->next = NULL;
  current = co_start("main", NULL, NULL); 
  current->state = CO_RUNNING; 
}

__attribute__((destructor))static void End()
{
  while(head->next != NULL)co_free(head->next);
  co_free(head);
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) 
{
  if(DEBUG) printf("start\n");
	struct co *thd = malloc(sizeof(struct co)); 
  thd->name = name;
	thd->func = func;
	thd->arg = arg;
	thd->state = CO_NEW;
  thd->next = head->next;
  head->next = thd;
  tot++;
  return thd;
}

void Finish()
{
	current->state = CO_DEAD;
	struct co *temp = head;
	while(temp->next != NULL)temp = temp->next;
	current = temp;
	longjmp(current->buf, 1);
}

struct co* get_co(){

  if(tot > 0)
  { 
    struct co *temp = head;
    int Choose = rand() % tot + 1;
    while(Choose > 0)
    {
      temp = temp->next;
      Choose--;
    }
    if(temp->state == CO_DEAD)
    {
      temp = head->next;
      while(temp != NULL && temp->state == CO_DEAD)temp = temp->next; 
    }
    return temp;
  }
  else return NULL;
  
}

void co_yield() {

  if(DEBUG) printf("yield\n");
  if(!setjmp(current->buf)){
    current = get_co();
  
    if(current->state == CO_NEW){
      current->state = CO_RUNNING;
      uintptr_t temp = (uintptr_t)current->stack + SZ_STACK - 1;
		  temp = temp - MASK;
		  uintptr_t retq = (uintptr_t)Finish;
		  memcpy((char *)temp, &retq, MASK + 1);
      if(DEBUG) printf("stack_ch\n");
      stackEX((void *)temp, current->func, (uintptr_t)current->arg);
    }else{
      longjmp(current->buf, 1);
    }
  }
}

void co_wait(struct co* co) {
  if(DEBUG)printf("wait\n");
  while(co->state != CO_DEAD)co_yield();
  co_free(co);

}

static void co_free(struct co* co) {

  struct co* cp = head;
  while (cp->next) {
    if(cp->next == co){
      cp->next = co->next;
      free(co);
      tot--;
      break;  
    }
    cp = cp->next;
  }
}


