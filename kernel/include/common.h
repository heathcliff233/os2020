#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

void spin_init(pthread_mutex_t* lk) {
	lk->locked = 0;
}