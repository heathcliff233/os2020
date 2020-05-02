#include <common.h>

static spinlock_t tasklock;
task_t* current_tasks[MAX_CPU] = {};
task_t* tasks[MAX_CPU][MAX_TASK] = {};
static int task_cnt[MAX_CPU] = {};

static _Context* kmt_context_save(_Event e, _Context* c);
static _Context* kmt_schedule(_Event e, _Context* c);

static void kmt_init() {
  spinlock_self();
  for (int i=0; i<MAX_CPU; i++) {
    current_tasks[i] = NULL;
    for (int j=0; j<MAX_TASK; j++) {
      tasks[i][j] = NULL;
    }
    task_cnt[i] = 0;
  }
  //putstr("before on irq\n");
  os->on_irq(0, _EVENT_NULL, kmt_context_save);
  os->on_irq(4, _EVENT_NULL, kmt_schedule); 
  //utstr("on irq finish\n");
  kmt->spin_init(&tasklock, "kmt_task");
}

static _Context* kmt_context_save(_Event e, _Context* c) {
  putstr("in context save\n");
  kmt->spin_lock(&tasklock);

  if (current_tasks[_cpu()] != NULL) {
    *current_tasks[_cpu()]->context = *c;
  }

  kmt->spin_unlock(&tasklock);
  putstr("finish context save\n");
  return NULL;
}

static _Context* kmt_schedule(_Event e, _Context* c) {
  putstr("in context schedule\n");
  kmt->spin_lock(&tasklock);
  _Context* ret = c;
  int valid_cnt = 0;
  task_t* valid_task[MAX_TASK];
  for(int i=0; i<MAX_TASK; i++) {
    if(tasks[_cpu()][i]!=NULL) {
      valid_task[valid_cnt] = tasks[_cpu()][i];
      valid_cnt++;
    }
  }

  if (valid_cnt!=0) {
    current_tasks[_cpu()] = valid_task[rand()%valid_cnt];
    ret = current_tasks[_cpu()]->context;
  }

  kmt->spin_unlock(&tasklock);
  //putstr("finish schedule");
  return ret;
}

static int kmt_create(task_t* task, const char* name, void (*entry)(void* arg), void* arg) {
  putstr("kmt created ");
  putstr("name\n");
  task->name = name;
  _Area stack =
    {(void*)task->stack, 
    (void*)((&task->stack) + STACK_SIZE)
  };
  task->context = _kcontext(stack, entry, arg);

  kmt->spin_lock(&tasklock);
  int min = MAX_TASK+1;
  int pivot = -1;
  for (int i=0; i<_ncpu(); i++) {
    if (task_cnt[i] <= min) {
      min = task_cnt[i];
      pivot = i;
    }
  }
  task_cnt[pivot] += 1;
  for (int j=0; j<MAX_TASK; j++) {
    if (tasks[pivot][j]==NULL) {
      task->cpu = pivot;
      tasks[pivot][j] = task;
      break;
    }
    assert(j<MAX_TASK);
  }
  kmt->spin_unlock(&tasklock);
  putstr("created!\n");
  return 0;
}

static void kmt_teardown(task_t* task) {
  kmt->spin_lock(&tasklock);
  int tar_cpu = task->cpu;
  for(int i=0; i<MAX_TASK; i++) {
    if(tasks[tar_cpu][i] == task) {
      tasks[tar_cpu][i] = NULL;
    }
  }
  kmt->spin_unlock(&tasklock);
}

MODULE_DEF(kmt) = {
  .init        =kmt_init,
  .create      =kmt_create,
  .teardown    =kmt_teardown,
  .spin_init   =spinlock_init,
  .spin_lock   =spinlock_acquire,
  .spin_unlock =spinlock_release,
  .sem_init    =kmt_sem_init,
  .sem_wait    =kmt_sem_wait,
  .sem_signal  =kmt_sem_signal
};