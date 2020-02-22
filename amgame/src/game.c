#include <game.h>
#include <klib.h>

#define FPS 10

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
    // print_key();
    int key = 0;
    // clear_screen();
    while ((key = read_key()) != _KEY_NONE) {
      clear_screen();
      //kbd_event(key);         // 处理键盘事件
      if (key == _KEY_ESCAPE) {
        _halt(0);
      }
    }
    splash();
    next_frame += 1000 / FPS; // 计算下一帧的时间
  }
  return 0;
}
