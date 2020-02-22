#include <game.h>
#include <klib.h>

#define FPS 1

// Operating system is a C program!
int main(const char *args) {
  _ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  int next_frame = 0;
  while (1) {
    while(uptime() < next_frame);
    int key = _KEY_NONE;
    splash();
    while (key == _KEY_NONE) {
      key = read_key();
    }

    if (key == _KEY_ESCAPE) {
      _halt(0);
    }
    clear_screen();

    next_frame += 1000 / FPS; // 计算下一帧的时间
  }
  return 0;
}
