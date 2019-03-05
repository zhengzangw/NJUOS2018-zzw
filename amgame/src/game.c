#include <game.h>
#include <klib.h>

#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)
#define UPDATE(player) \
    do { \
      player.dx += player.ddx; player.dy += player.ddy; player.x += player.dx; player.y += player.dy; \
    } while(0)
const int FPS = 10;
static int width, height, next_frame, key;

struct Item {
    int ddx, ddy, dx, dy, x, y;
};
struct Item player ={
    .x = 0,
    .y = 0,
    .dy = 0,
    .ddx = 0,
    .dx = 1,
    .ddy = 1
};

uint32_t black[1000];
void clear_rect(int x, int y, int w, int h)
{
        draw_rect(black, x, y, w, h);
}

void game_progress()
{
}

int x = 0, y = 0;
void screen_update()
{
        uint32_t pixels[100];
        for (int i = 0; i < 100; ++i)
                pixels[i] = 0xff00ff;
        clear_rect(player.x, player.y, 10, 10);
        UPDATE(player);
        draw_rect(pixels, player.x, player.y, 10, 10);
}

int main()
{
        // Operating system is a C program
        _ioe_init();
        width = screen_width();
        height = screen_height();

        player.x = width / 3;
        player.y = height / 3;

        while (1) {
                while (uptime() < next_frame) ;
                while ((key = read_key()) != _KEY_NONE) {
                    if (KEYCODE(key)==_KEY_SPACE){
                        if (ISKEYDOWN(key))
                            player.dy = -10;
                    }
                }
                game_progress();
                screen_update();
                next_frame += 1000 / FPS;
        }

        return 0;
}
