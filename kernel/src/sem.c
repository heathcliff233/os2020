#include <common.h>

extern task_t* current_tasks[MAX_CPU];

void kmt_sem_init(sem_t* sem, const char* name, int value) {
  kmt->spin_init(&sem->lock, name);
  sem->value = value;
  sem->tail = sem->head = 0;
  for (int i=0; i<MAX_TASK; i++) sem->pool[i] = NULL;
}

void kmt_sem_wait(sem_t* sem) {
  kmt->spin_lock(&sem->lock);
  if (sem->value <= 0) {
    sem->pool[sem->tail] = current_tasks[_cpu()];
    sem->tail = (sem->tail + 1) % MAX_TASK;
  }
  while(sem->value <= 0) {
    kmt->spin_unlock(&sem->lock);

    _yield();
    kmt->spin_lock(&sem->lock);
  }
  __sync_synchronize();
  sem->value--;
  kmt->spin_unlock(&sem->lock);
}

void kmt_sem_signal(sem_t* sem) {
  kmt->spin_lock(&sem->lock);
  sem->value++;
  if(sem->pool[sem->head]){
      sem->pool[sem->head] = NULL;
      sem->head = (sem->head + 1) % MAX_TASK;
  }
  kmt->spin_unlock(&sem->lock);

}