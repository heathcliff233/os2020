#include <common.h>

typedef struct irq{
    int seq,event;
    handler_t handler;
    struct irq *next;
} irq_handler_t;

static irq_handler_t root_handler = {
  0, _EVENT_NULL, NULL, NULL
};

static void os_init() {
  pmm->init();
  kmt->inti();
}

static void os_run() {
  _intr_write(1);
  while (1) ;
}

static _Context *os_trap(_Event ev, _Context *ctx) {
  _Context *next = NULL;
  for(irq_handler_t* h = root_handler; h!=NULL; h=h->next) {
  //for (auto &h: handlers_sorted_by_seq) {
    if (h->event == _EVENT_NULL || h->event == ev.event) {
      _Context *r = h->handler(ev, ctx);
      panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
  }
  panic_on(!next, "returning NULL context");
  //panic_on(sane_context(next), "returning to invalid context");
  return next;
}

static void os_on_irq(int seq, int event, handler_t handler) {
    irq_handler_t *prev=&root_handler,*p=(&root_handler)->next;
    while(p){
        if(p->seq>seq||p==&root_handler)break;
        prev=p;
        p=p->next;
    }
    prev->next=new(root_handler);
    prev=prev->next;

    prev->seq=seq;
    prev->event=event;
    prev->handler=handler;
    prev->next=p;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
