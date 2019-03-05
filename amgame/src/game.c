#include <game.h>
#include <klib.h>

#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)
const int FPS = 1;
static int weight, height, next_frame, key;

struct Player {
    int dx, dy;
    int x,y;
} player;

uint32_t black[1000];
void clear_rect(int x, int y, int w, int h){
    draw_rect(black, x, y, w, h);
}

void game_progress(){
}

int x=0, y=0;
void screen_update(){
  uint32_t pixels[100];
  for (int i=0;i<100;++i) pixels[i] = 0xff00ff;
  clear_rect(player.x, player.y, 10, 10);
  player.x += player.dx; player.y += player.dy; player.dx = player.dy = 0;
  draw_rect(pixels, player.x, player.y, 10, 10);
}

int main() {
  // Operating system is a C program
  _ioe_init();
  weight = screen_width();
  height = screen_height();

  player.x = weight/2;
  player.y = height/2;

  while (1) {
    while (uptime() < next_frame);
    while ((key = read_key())!=_KEY_NONE && ISKEYDOWN(key)){
        switch KEYCODE(key){
          case _KEY_W: player.dx=1; break;
          default: break;
        }
    }
    game_progress();
    screen_update();
    next_frame += 1000/FPS;
  }

  return 0;
}

