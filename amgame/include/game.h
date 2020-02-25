#include <am.h>
#include <amdev.h>

void splash();
void clear_screen();
void print_key();
static inline void puts(const char *s) {
  for (; *s; s++) _putc(*s);
}
void init_screen();
void clear_screen();
void draw_rect_pure(int x, int y, int w, int h, uint32_t color);
void draw_char(char c, int x, int y, int color_char, int color_back);
void draw_string(const char *str, int x, int y, int color_char, int color_back);

