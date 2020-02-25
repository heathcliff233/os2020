#include <game.h>
#include <klib.h>
#include <fonts.h>

#define SIDE 16
#define char_p 1
#define char_w 8
#define char_h 8
#define char_space 1

int w, h, p;

static void init() {
  _DEV_VIDEO_INFO_t info = {0};
  _io_read(_DEV_VIDEO, _DEVREG_VIDEO_INFO, &info, sizeof(info));
  w = info.width;
  h = info.height;
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // careful! stack is limited!
  _DEV_VIDEO_FBCTRL_t event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  _io_write(_DEV_VIDEO, _DEVREG_VIDEO_FBCTRL, &event, sizeof(event));
}

/*
void splash() {
  init();
  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); // white
      }
    }
  }
}
*/

/*
void clear_screen() {
  init();
  draw_tile(0, 0, w, h, 0xffffff);
}
*/

void init_screen()
{
    init();

    p = w > h ? h : w;
    p /= 200;
    if (p < 1)
    {
        p = 1;
    }

    clear_screen();

    //printf("Init with w = %d, h = %d\n", w, h);
}

void clear_screen()
{
    draw_rect_pure(0, 0, w, h, 0x0);
    draw_sync();
}
/*
static void draw_rect_small(int x, int y, int wt, int ht, uint32_t color)
{
    if (x + wt > w || y + ht > h)
    {
        return;
    }
    uint32_t pixels[wt * ht]; // WARNING: allocated on stack
    for (int i = 0; i < wt * ht; i++)
    {
        pixels[i] = color;
    }
    draw_rect(pixels, x, y, wt, ht);
}
*/
void draw_rect_pure(int x, int y, int w, int h, uint32_t color)
{
    for (int ix = 0; ix * SIDE <= w; ix++)
    {
        int dx = w - ix * SIDE;
        if (dx > SIDE)
        {
            dx = SIDE;
        }

        for (int iy = 0; iy * SIDE <= h; iy++)
        {
            int dy = h - iy * SIDE;
            if (dy > SIDE)
            {
                dy = SIDE;
            }
            //draw_rect_small(x + ix * SIDE, y + iy * SIDE, dx, dy, color);
            draw_tile(x + ix * SIDE, y + iy * SIDE, dx, dy, color);
        }
    }
}

void draw_char(char ch, int x, int y, int color_char, int color_black) {
  int i, j;
  char *p = fonts[(int)ch];
  for (i = 0; i < 8; i ++) 
    for (j = 0; j < 8; j ++) 
      if ((p[i] >> j) & 1){
          draw_tile(x+j,y+i,1,1,0xffffff);
      }else{
          draw_tile(x+j,y+i,1,1,0x0);
      }
}

void draw_string(const char *str, int x, int y, int color_char, int color_back)
{
    //int len = strlen(str);
    draw_rect_pure(x, y, 4 * char_p * p * (char_w + char_space), char_h * char_p * p * 2, color_back);
    /*
    for (int i = 0; i < len; i++)
    {
        draw_char(str[i], x + i * char_p * p * (char_w + char_space), y, color_char, color_back);
    }
    */
    int pivot = 0;
    while(str[pivot] != '\0'){
      draw_char(str[pivot], x+8*pivot , y, color_char, color_back);
      pivot ++;
    }
}

