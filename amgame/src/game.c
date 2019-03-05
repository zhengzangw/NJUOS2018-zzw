#include <game.h>
#include <klib.h>
#include <time.h>
#include <stdlib.h>

//Global Define
#define W 0xffffff
#define B 0x000000
#define R 0xff0000
#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)
#define UPDATE(item) \
    do { \
      item->dx += item->ddx; item->dy += item->ddy; item->x += item->dx; item->y += item->dy; \
    } while(0)
#define clear_rect(x,y,w,h) draw_rect(black, x, y, w, h)
#define FPS 10;
#define VECT 12;
static int width, height, next_frame, key;
uint32_t black[1000],white[1000];
uint32_t player_pixels[2][400] ={HEART1,HEART2};

struct Item {
    int ddx, ddy, dx, dy, x, y, w, h, valid;
};
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

void screen_update_player(struct Item* player){
    static int sw = 0;
    sw = (sw + 1)%8;
    clear_rect(player->x, player->y, player->w, player->h);
    UPDATE(player);
    draw_rect(player_pixels[sw<4], player->x, player->y, player->w, player->h);
}

void screen_update_obs(struct Item* obs){
    clear_rect(obs->x, obs->y, obs->w, obs->h);
    UPDATE(obs);
    draw_rect(white, obs->x, obs->y, obs->w, obs->h);
}

void screen_update()
{
    screen_update_obs(&obs[0]);
    screen_update_player(&player);
}

// Game Progress
void init_obs(struct Item* obs){
    obs->x = width;
    obs->y = 0;
    obs->dx = -5;
    obs->dy = obs->ddx = obs->ddy =0;
    obs->w = 3;
    obs->h = rand()%300+100;
    Log("%d", obs->h);
}

void game_progress()
{
    if (head_obs==0) init_obs(&obs[head_obs++]);
}

int main()
{
        // Operating system is a C program
        // Initialization
        _ioe_init();
        srand(100);
        for (int i=0;i<1000;++i){ white[i] = R; }
        width = screen_width();
        height = screen_height();
        player.x = width / 3;
        player.y = height / 3;

        // Game Start
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
