#include <game.h>
#include <klib.h>
#include <time.h>
#include <stdlib.h>

#define W 0xffffff
#define B 0x000000
#define R 0xff0000
#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)
#define RAND(x) (rand()%(x))
#define UPDATE(item) \
    do { \
      item->dx += item->ddx; item->dy += item->ddy; item->x += item->dx; item->y += item->dy; \
    } while(0)
const int FPS = 10;
const int VECT = 12;
static int width, height, next_frame, key;

struct Item {
    int ddx, ddy, dx, dy, x, y, w, h;
};
uint32_t player_pixels[2][400] ={{
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,
    W, W, R, R, R, R, R, R, R, R, W, W, R, R, R, R, W, W, B, B,
    W, W, R, R, R, R, R, R, R, R, W, W, R, R, R, R, W, W, B, B,
    W, W, R, R, R, R, R, R, R, R, R, R, R, R, R, R, W, W, B, B,
    W, W, R, R, R, R, R, R, R, R, R, R, R, R, R, R, W, W, B, B,
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B
    },{
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,
    B, B, B, B, W, W, W, W, B, B, W, W, W, W, B, B, B, B, B, B,
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,
    B, B, W, W, R, R, R, R, W, W, R, R, R, R, W, W, B, B, B, B,
    W, W, R, R, W, W, W, W, R, R, R, R, R, R, R, R, W, W, B, B,
    W, W, R, R, W, W, W, W, R, R, R, R, R, R, R, R, W, W, B, B,
    W, W, R, R, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B,
    W, W, R, R, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B,
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,
    B, B, W, W, R, R, R, R, R, R, R, R, R, R, W, W, B, B, B, B,
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,
    B, B, B, B, W, W, R, R, R, R, R, R, W, W, B, B, B, B, B, B,
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, W, W, R, R, W, W, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, W, W, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B
    }};
struct Item player ={
    .x = 0,
    .y = 0,
    .dy = 0,
    .ddx = 0,
    .dx = 0,
    .ddy = 1,
    .w = 20,
    .h = 20
};
int num_obs, head_obs;
struct Item obs[5];

uint32_t black[1000];
void clear_rect(int x, int y, int w, int h)
{
        draw_rect(black, x, y, w, h);
}

void screen_update_player(struct Item* player){
    static int sw = 0;
    sw = (sw + 1)%8;
    clear_rect(player->x, player->y, player->w, player->h);
    UPDATE(player);
    draw_rect(player_pixels[sw<4], player->x, player->y, player->w, player->h);
}

uint32_t white[1000];
void screen_update_obs(struct Item* obs){
    clear_rect(obs->x, obs->y, obs->w, obs->h);
    UPDATE(obs);
    draw_rect(white, obs->x, obs->y, obs->w, obs->h);
}


void init_obs(struct Item* obs){
    obs->x = width-10;
    obs->y = 10;
    obs->dx = -5;
    obs->dy = obs->ddx = obs->ddy =0;
    obs->w = 3;
    obs->h = RAND(100);
}
void game_progress()
{
}

int x = 0, y = 0;
void screen_update()
{
    screen_update_obs(&obs[head_obs]);
    screen_update_player(&player);
}

int main()
{
        // Operating system is a C program
        _ioe_init();
        srand(time(NULL));
        for (int i=0;i<1000;++i){ white[i] = R; }
        width = screen_width();
        height = screen_height();

        player.x = width / 3;
        player.y = height / 3;

        init_obs(&obs[head_obs]);
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
