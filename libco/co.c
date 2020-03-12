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


#define DEBUG false
#define SZ_STACK 16*4096
#define NR_CO 16

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

#define stackEX(newsp, backup) \
  asm volatile("mov " SP ", %0; mov %1, " SP : "=g"(backup) : "g"(newsp))
#define getSP(sp) \
  asm volatile("mov " SP ", %0" : "=g"(sp) : )

struct co {
  int state;
  char name[32];
  void (*func)(void *);
  void* arg;
  struct co* next;
  jmp_buf buf;
  char stack[SZ_STACK];
  void* stack_ptr;
};

static void* stack_backup;
static jmp_buf start_buf;
static jmp_buf wait_buf;
struct co* head = NULL;
struct co* current = NULL;

static struct co* co_create(const char *name, void (*func)(void *), void *arg);
static void co_free(struct co* co);

struct co* co_start(const char *name, void (*func)(void *), void *arg) {
  if(DEBUG) printf("Start co %s\n", name);
  current = co_create(name, func, arg);
  
  if(DEBUG) printf("created but no context switch\n");
  if(!setjmp(start_buf)) {
    stackEX(current->stack_ptr, stack_backup);
    current->func(current->arg);
    longjmp(wait_buf, 1);

  } else {
    if(DEBUG) printf("init finished\n");
  }
  
  return current;
}

void co_yield() {
  if(!setjmp(current->buf)) {
    if(current->state == CO_NEW) {
      stackEX(stack_backup, current->stack_ptr);
      current->state = CO_WAITING;
      longjmp(start_buf, 1);

    } else {
      struct co* next = current->next ? current->next : head->next;
      current->state = CO_WAITING;
      stackEX(next->stack_ptr, current->stack_ptr);
      current = next;
      longjmp(current->buf, 1);
    }
  } else {
    stackEX(current->stack_ptr, stack_backup);
    current->state = CO_RUNNING;
    if(DEBUG) printf("go to thread %s \n",current->name);
  }
}

void co_wait(struct co* co) {
  if(DEBUG) printf("wait for %s \n", co->name);
  while(co->state != CO_RUNNING) {
    if(!setjmp(wait_buf)) {
      current = co;
      longjmp(co->buf, 1);
    }
    if(current == co) break;
  }
  co->state = CO_DEAD;
  co_free(co);
  return;
}

static struct co* co_create(const char *name, void (*func)(void *), void *arg) {
  struct co* ret = (struct co*) malloc(sizeof(struct co));
  ret->state = CO_NEW;
  strncpy(ret->name, name, sizeof(ret->name));
  ret->func = func;
  ret->arg = arg;
  ret->next = NULL;
  ret->stack_ptr = (void*)((((intptr_t)ret->stack + sizeof(ret->stack))>>4)<<4);
  if(head) {
    struct co* cp = head;
    while (cp->next) {
      cp = cp->next;
    }
    cp->next = ret;
  } else {
    head = ret;
  }
  return ret;
}

static void co_free(struct co* co) {
  if(!head) return;
  if(head == co) {
    struct co* next = head->next;
    free(head);
    head = next;
  }
  if(head) {
    struct co* cp = head;
    while (cp->next) {
      if(cp->next == co){
        struct co* next = cp->next->next;
        free(cp->next);
        break;
      }
      cp = cp->next;
    }
  }
}
