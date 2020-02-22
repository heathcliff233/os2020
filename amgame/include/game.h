#include <am.h>
#include <amdev.h>

void splash();
void clear_screen();
void print_key();
static inline void puts(const char *s) {
  for (; *s; s++) _putc(*s);
}
