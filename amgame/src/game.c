#include <game.h>
#include <klib.h>

#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)
const int FPS = 1;
static int weight, height, next_frame, key;

void game_progress(){
}

int x=0, y=0;
void screen_update(){
  uint32_t pixels[1];
  for (int i=0;i<1;++i) pixels[i] = 0xff00ff;
  draw_rect(pixels, x++, y++, 1, 1);
}

int main() {
  // Operating system is a C program
  _ioe_init();
  weight = screen_width();
  height = screen_height();

  while (1) {
    while (uptime() < next_frame);
    while ((key = read_key())!=_KEY_NONE && ISKEYDOWN(key)){
        switch KEYCODE(key){
          case _KEY_W: printf("Hello\n"); break;
          default: break;
        }
    }
    game_progress();
    screen_update();
    next_frame += 1000/FPS;
  }

  return 0;
}

