#include <game.h>
#include <klib.h>

#define FPS 10
#define W 0xffffff
#define B 0x000000
#define R 0xff0000
#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)

int game_map[4][4] = {0};
int change;

void game_init();
void draw_num(int i, int j);
void forward(int key);
void draw();
int generate();

// Operating system is a C program!
int main(const char *args) {
  _ioe_init();
  int next_frame = 0;
  int is_halt = 0;
  game_init();
  while (1) {
    while(uptime() < next_frame);
    int key = _KEY_NONE;
    while ((key=read_key()) != _KEY_NONE) {
      if(ISKEYDOWN(key)){
        key = KEYCODE(key);
        if(key == _KEY_ESCAPE){
          is_halt = 1;
        }else if(key==_KEY_A || key==_KEY_W || key==_KEY_S || key==_KEY_D){
          forward(key);
        }
      }
      
    }
    if (is_halt == 1) {
      _halt(0);
    }

    next_frame += 1000 / FPS; // 计算下一帧的时间
  }
  return 0;
}

/*
|-----|-----|-----|-----|

*/

void game_init(){
  for(int i=0; i<4; i++){
    for(int j=0; j<4; j++){
      game_map[i][j] = 0;
    }
  }
  int x = 1;
  int y = 1;
  draw_char('a',0,0,R,W);
  game_map[y][x] = 2;
  draw();
}

void draw(){
  for (int i=0; i<4; i++){
    for (int j=0; j<4; j++){
      draw_num(i,j);
    }
  }
}

void draw_num(int i, int j){
  int num = game_map[i][j];
  int y = 3*8*i;
  int x = 5*8*j;
  char c[6];
  int lst;
  for(int id = 0; id<5; id++){
    if(id==0 && num==0){
      c[0] = '0';
      c[1] = '\0';
      break;
    }
    if(num > 0){
      lst = num%10;
      c[id] = lst + '0';
      num /= 10;
    
    }else{
      c[id] = '\0';
      break;
    }
  }
  draw_rect_pure(x, y, 5*8, 8, B);
  draw_string(c, x, y, W, B);

}

void forward(int key){
  change = 0;
  int x, y, sign;
  switch(key) {
    case _KEY_D : 
      for(y=0; y<4; y++){
        for(sign=2; sign>=0; sign--){
          x = sign;
          while(x<3 && (game_map[y][x+1]==0 || game_map[y][x]==game_map[y][x+1])){
            change = 1;
            if(game_map[y][x+1]==0){
              game_map[y][x+1]=game_map[y][x];
            }else{
              game_map[y][x+1] *= 2;
            }
            game_map[y][x] = 0;
            x++;
          }
        }
      }break;

    case _KEY_A :
      for (y=0; y<4; y++){
        for(sign=1; sign<4; sign++){
          x = sign;
          while(x>0 && (game_map[y][x-1]==0 || game_map[y][x]==game_map[y][x-1])){
            change = 1;
            if(game_map[y][x-1]==0){
              game_map[y][x-1] = game_map[y][x];
            }else{
              game_map[y][x-1] *= 2;
            }
            game_map[y][x] = 0;
            x--;
          }
        }
      }break;

    case _KEY_W :
      for (x=0; x<4; x++){
        for(sign=1; sign<4; sign++){
          y = sign;
          while(y>0 && (game_map[y-1][x]==0 || game_map[y][x]==game_map[y-1][x])){
            change = 1;
            if(game_map[y-1][x]==0){
              game_map[y-1][x] = game_map[y][x];
            }else{
              game_map[y-1][x] *= 2;
            }
            game_map[y][x] = 0;
            y--;
          }
        }
      }break;

    case _KEY_S :
      for(x=0; x<4; x++){
        for(sign=2; sign>=0; sign--){
          y = sign;
          while(y<3 && (game_map[y+1][x]==0 || game_map[y][x]==game_map[y+1][x])){
            change = 1;
            if(game_map[y+1][x]==0){
              game_map[y+1][x]=game_map[y][x];
            }else{
              game_map[y+1][x] *= 2;
            }
            game_map[y][x] = 0;
            y++;
          }
        }
      }break;

  }
  if(change == 0){
    return;
  }
  int ret = generate();
  if(ret==0){
    clear_screen();
  }else{
    draw();
  }
}

int generate(){
  int x, y;
  for (y=0; y <4; y++){
    for (x=0; x<4; x++){
      if (game_map[y][x] == 0){
        game_map[y][x] = 2;
        return 1;
      }
    }
  }
  return 0;
}