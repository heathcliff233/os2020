#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

#define MAX_CPU 8

//============ define mutex lock =============
typedef uintptr_t mutex_t;
#define MUTEX_INITIALIZER 0
void mutex_lock(mutex_t* locked);
void mutex_unlock(mutex_t* locked);

typedef struct spinlock {
    const char* name;
    int owner;
    mutex_t lock;
} spinlock_t;

void spinlock_init(spinlock_t* lk, const char* name);
int spinlock_holding(spinlock_t* lk);
void spinlock_acquire(spinlock_t* lk);
void spinlock_release(spinlock_t* lk);
//============ end define ====================

#define STACK_SIZE 4096
#define MAX_TASK 24

typedef struct task {
    int valid;
    const char* name;
    _Context* context;
    uint8_t stack[STACK_SIZE];
    int cpu;
} task_t;

typedef struct semaphore {
    volatile int value;
    spinlock_t lock;
    task_t* pool[MAX_TASK];
    volatile int head,tail;
} sem_t;

