#include <common.h>

typedef struct irq{
    int seq,event;
    handler_t handler;
    int valid;
    //struct irq *next;
} irq_handler_t;
/*
static irq_handler_t root_handler = {
  0, _EVENT_NULL, NULL, NULL
};
*/

void echo_test(void* arg) {
  while(1) putstr("ass");
}
//===========================================================
#define INT_SEQ_MIN 0
#define INT_SEQ_MAX 4
#define INT_NR_MAX 4
static irq_handler_t trap_handlers[INT_SEQ_MAX][INT_NR_MAX];
static _Context* trap_handler_invalid(_Event e, _Context* c) {
    // should not reach here
    assert(0);
}
static void trap_init() {
    for (int i = INT_SEQ_MIN; i < INT_SEQ_MAX; i++) {
        for (int j = 0; j < INT_NR_MAX; j++) {
            trap_handlers[i][j].handler = trap_handler_invalid;
            trap_handlers[i][j].valid = 0;
        }
    }
}
//============================================================
static void os_init() {
  pmm->init();
  kmt->init();

  trap_init();

//#ifdef DEBUG
/*
  kmt->sem_init(&empty, "empty", 5);  // 缓冲区大小为 5
  kmt->sem_init(&fill,  "fill",  0);
  for (int i = 0; i < 4; i++) // 4 个生产者
    kmt->create(task_alloc(), "producer", producer, NULL);
  for (int i = 0; i < 5; i++) // 5 个消费者
    kmt->create(task_alloc(), "consumer", consumer, NULL);
*/
  for(int i=0; i<5; i++) {
    kmt->create(pmm->alloc(sizeof(task_t)), "shit", echo_test, "a");
  }
//#endif
  putstr("after os init and kmt create\n");
}

static void os_run() {
  _intr_write(1);
  while(1);
  panic("shit failed!\n");
}

/*
static _Context *os_trap(_Event ev, _Context *ctx) {
  _Context *next = NULL;
  for(irq_handler_t* h = (&root_handler)->next; h!=NULL; h=h->next) {
    if (h->event == _EVENT_NULL || h->event == ev.event) {
      _Context *r = h->handler(ev, ctx);
      panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
  }
  panic_on(!next, "returning NULL context");
  return next;
}

static void os_on_irq(int seq, int event, handler_t handler) {
    irq_handler_t *prev=&root_handler,*p=(&root_handler)->next;
    
    while(p){
        if(p->seq>seq||p==&root_handler)break;
        prev=p;
        p=p->next;
    }
    
    prev->next = pmm->alloc(sizeof(handler_t));
    prev=prev->next;
    
    prev->seq=seq;
    prev->event=event;
    prev->handler=handler;
    prev->next=p;
}
*/

//==============================================================
static _Context* os_trap(_Event ev, _Context* context) {
    // util_log("TRAP", trap_nr[_cpu()]++, LOG_WARNING, LOG_NHEX);
    _Context* ret = NULL;
    for (int i = INT_SEQ_MIN; i < INT_SEQ_MAX; i++) {
        for (int j = 0; j < INT_NR_MAX; j++) {
            if (trap_handlers[i][j].valid) {
                if (trap_handlers[i][j].event == ev.event ||
                    trap_handlers[i][j].event == _EVENT_NULL) {
                    _Context* next = trap_handlers[i][j].handler(ev, context);
                    // util_log("Trap succ", (uint32_t)next, LOG_SUCCESS,
                    //          LOG_NHEX);
                    if (next != NULL) {
                        ret = next;
                    }
                }
            } else {
                break;
            }
        }
    }
    if (ret == NULL) {
        ret = context;
    }
    return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
    int ptr;
    for (ptr = INT_SEQ_MIN; ptr < INT_NR_MAX; ptr++) {
        if (trap_handlers[seq][ptr].valid == 0) {
            break;
        }
    }
    if (ptr == INT_NR_MAX) {
        // No free slot, should enlarge INT_NR_MAX
        assert(0);
        return;
    }
    trap_handlers[seq][ptr].valid = 1;
    trap_handlers[seq][ptr].seq = seq;
    trap_handlers[seq][ptr].event = event;
    trap_handlers[seq][ptr].handler = handler;
    printf("finish on_irq\n");
    // util_log("Trap init", trap_handlers[seq][ptr].valid, LOG_SUCCESS,
    // LOG_NHEX); util_log("Trap init", seq, LOG_SUCCESS, LOG_NHEX);
    // util_log("Trap init", ptr, LOG_SUCCESS, LOG_NHEX);
}
//==============================================================

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_on_irq,
};
