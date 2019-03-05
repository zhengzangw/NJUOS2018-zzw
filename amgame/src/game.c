#include <game.h>
#include <klib.h>

#define W 0xffffff
#define B 0x000000
#define R 0xff0000
#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)
#define UPDATE(player) \
    do { \
      player.dx += player.ddx; player.dy += player.ddy; player.x += player.dx; player.y += player.dy; \
    } while(0)
const int FPS = 10;
const int VECT = 10;
static int width, height, next_frame, key;

struct Item {
    int ddx, ddy, dx, dy, x, y;
};
uint32_t player_pixels[2][100] ={{
    B, B, B, B, B, B, B, B, B, B,
    B, B, W, W, B, W, W, B, B, B,
    B, W, R, R, W, R, R, W, B, B,
    W, R, R, R, R, W, R, R, W, B,
    W, R, R, R, R, R, R, R, W, B,
    B, W, R, R, R, R, R, W, B, B,
    B, B, W, R, R, R, W, B, B, B,
    B, B, B, W, R, W, B, B, B, B,
    B, B, B, B, W, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B
    },{
    B, B, B, B, B, B, B, B, B, B,
    B, B, W, W, B, W, W, B, B, B,
    B, W, R, R, W, R, R, W, B, B,
    W, R, W, W, R, R, R, R, W, B,
    W, R, W, R, R, R, R, R, W, B,
    B, W, R, R, R, R, R, W, B, B,
    B, B, W, R, R, R, W, B, B, B,
    B, B, B, W, R, W, B, B, B, B,
    B, B, B, B, W, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B
    }};
struct Item player ={
    .x = 0,
    .y = 0,
    .dy = 0,
    .ddx = 0,
    .dx = 0,
    .ddy = 1
};

uint32_t black[1000];
void clear_rect(int x, int y, int w, int h)
{
        draw_rect(black, x, y, w, h);
}

void screen_update_player(){
    int sw = 0;
    if (player.ddy<0) sw=1;
    clear_rect(player.x, player.y, 10, 10);
    UPDATE(player);
    draw_rect(player_pixels[sw], player.x, player.y, 10, 10);
}


void game_progress()
{
}

int x = 0, y = 0;
void screen_update()
{
    screen_update_player();
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
                            player.dy = -VECT;
                    }
                }
                game_progress();
                screen_update();
                next_frame += 1000 / FPS;
        }

        return 0;
}
