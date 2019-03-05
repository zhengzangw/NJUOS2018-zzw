#include <game.h>
#include <klib.h>

#define ISKEYDOWN(x) (((x)&0x8000)==1)
const int FPS = 1;
static int weight, height, next_frame, key;

void init_screen();
void splash();

int main() {
  // Operating system is a C program
  _ioe_init();
  init_screen();

  splash();
  while (1) {
    while (uptime() < next_frame);
    printf("%d", key);
    while ((key = read_key())!=_KEY_NONE && ISKEYDOWN(key)){
        printf("hello\n");
    }
    next_frame += 1000/FPS;
  }

  return 0;
}

/*
void read_key() {
  _DEV_INPUT_KBD_t event = { .keycode = _KEY_NONE };
  #define KEYNAME(key) \
    [_KEY_##key] = #key,
  static const char *key_names[] = {
    _KEYS(KEYNAME)
  };
  _io_read(_DEV_INPUT, _DEVREG_INPUT_KBD, &event, sizeof(event));
  if (event.keycode != _KEY_NONE && event.keydown) {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
  }
}
*/

void init_screen() {
  _DEV_VIDEO_INFO_t info = {0};
  _io_read(_DEV_VIDEO, _DEVREG_VIDEO_INFO, &info, sizeof(info));
  weight = info.width;
  height = info.height;
}

void draw_srect(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: allocated on stack
  _DEV_VIDEO_FBCTL_t event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  _io_write(_DEV_VIDEO, _DEVREG_VIDEO_FBCTL, &event, sizeof(event));
}

void splash() {
  for (int x = 0; x * SIDE <= weight; x ++) {
    for (int y = 0; y * SIDE <= height; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_srect(x * SIDE, y * SIDE, SIDE, SIDE, 0xffff00); // white
      }
    }
  }
}
