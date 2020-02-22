#include <game.h>
#include <klib.h>

#define FPS 10
#define W 0xffffff
#define B 0x000000
#define R 0xff0000
#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)

uint32_t orange[1000];

// Operating system is a C program!
int main(const char *args) {
  _ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  int next_frame = 0;
  int is_halt = 0;
  int is_print = 0;
  while (1) {
    is_print = 0;
    while(uptime() < next_frame);
    int key = _KEY_NONE;
    for(int i=0;i<1000;++i) orange[i]=R;
    //splash();
    while (key=read_key != _KEY_NONE) {
      if (KEYCODE(key)==_KEY_ESCAPE){
        if(ISKEYDOWN(key)) is_halt = 1;
      }else{
        if(ISKEYDOWN(key)) is_print = 1;
      }
    }
    //int base = uptime() + 1000;
    if (is_halt == 1) {
      _halt(0);
    }
    //clear_screen();
    draw_rect(orange,0,0,20,20);
    //while(uptime() < base);

    next_frame += 1000 / FPS; // 计算下一帧的时间
  }
  return 0;
}
