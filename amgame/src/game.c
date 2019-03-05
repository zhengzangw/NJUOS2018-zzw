#include <game.h>
#include <klib.h>
#include <time.h>
#include <stdlib.h>

//Global Define
#define W 0xffffff
#define B 0x000000
#define R 0xff0000
#define ORANGE 0xff7D00
#define ISKEYDOWN(x) (((x)&0x8000))
#define KEYCODE(x) ((x)&0x7fff)
#define update(item) \
    do { \
      item->dx += item->ddx; item->dy += item->ddy; item->x += item->dx; item->y += item->dy; \
    } while(0)
#define clear_rect(x,y,w,h) draw_rect(black, x, y, w, h)
#define INBOUND(item) (((item)->x>=10)&&((item)->y>=0)&&((item)->x+(item)->w<=width)&&((item)->y+(item)->h<=height))
#define COLLIDE(player, item) ((player)->x+(player)->w>(item)->x && \
                               (item)->x  +(item)->w  >(player)->x && \
                               (player)->y+(player)->h>(item)->y && \
                               (item)->y  +(item)->h  >(player)->y)
#define FPS 10
#define VECT 10
#define GRAVITY 2
#define _PLAYER_FLASH 8
#define _OBS_FLASH 10
static int width, height, next_frame, key, fail, num_obs;
uint32_t black[1000],white[1000];
uint32_t player_pixels[2][400] ={HEART1,HEART2};

struct Item {
    int ddx, ddy, dx, dy, x, y, w, h, valid;
};
struct Item obs[10];
struct Item player;

// Screen Update
void screen_update_player(struct Item* player){
    static int counter = 0;
    counter = (counter + 1)%_PLAYER_FLASH;
    clear_rect(player->x, player->y, player->w, player->h);
    update(player);
    draw_rect(player_pixels[counter<4], player->x, player->y, player->w, player->h);
}

void screen_update_obs(struct Item* obs){
    clear_rect(obs->x, obs->y, obs->w, obs->h);
    update(obs);
    draw_rect(white, obs->x, obs->y, obs->w, obs->h);
}

void screen_update()
{
    screen_update_player(&player);
    for (int i=0;i<10;++i){
        if (obs[i].valid){
          screen_update_obs(&obs[i]);
        }
    }
}

// Game Progress
void init_player(struct Item* player){
    player->x = width / 3;
    player->y = height / 3;
    player->dy = 0;
    player->ddx = 0;
    player->dx = 0;
    player->ddy = GRAVITY;
    player->w = 20;
    player->h = 20;
    player->valid = 1;
};

void init_obs(struct Item* obs){
    obs->x = width-10;
    obs->y = rand()%(height/2);
    obs->dx = -(rand()%7)-3;
    obs->dy = obs->ddx = obs->ddy =0;
    obs->w = rand()%3+1;
    obs->h = rand()%(height/2)+height/4;
    obs->valid = 1;
}

void game_progress()
{
    static int counter = 0;
    int add = 0;
    counter = (counter + 1) % _OBS_FLASH;
    if (!counter && num_obs<8) add = 1;
    if (!INBOUND(&player)) {
        fail = 1;
        return;
    }

    for (int i=0;i<10;++i){
        if (obs[i].valid && COLLIDE(&player, &obs[i])) {
            fail = 1;
            return;
        }
        if (obs[i].valid){
            if (!INBOUND(&obs[i])){
                clear_rect(obs[i].x, obs[i].y, obs[i].w, obs[i].h);
                obs[i].valid = 0;
                num_obs--;
            }
        } else if (add){
              add = 0;
              init_obs(&obs[i]);
              num_obs++;
        }
    }
}

void restart(){
    clear_rect(player.x, player.y, player.w, player.h);
    for (int i=0;i<10;++i){
        if (obs[i].valid)
                clear_rect(obs[i].x, obs[i].y, obs[i].w, obs[i].h);
                obs[i].valid = 0;
    }
    init_player(&player);
    num_obs=0;
}

int main()
{
        // Operating system is a C program
        // Initialization
        _ioe_init();
        for (int i=0;i<1000;++i){ white[i] = ORANGE; }
        width = screen_width();
        height = screen_height();
        srand(100);

        // Game Start
        while (1) {
        restart();
        while (1) {
                while (uptime() < next_frame) ;
                while ((key = read_key()) != _KEY_NONE) {
                    if (KEYCODE(key)==_KEY_SPACE){
                        if (ISKEYDOWN(key)){
                            player.dy = -VECT;
                            player.ddy = -1;
                        }
                        else
                            player.ddy = GRAVITY;
                    }
                }
                game_progress();
                if (fail) break;
                screen_update();
                next_frame += 1000 / FPS;
        }
        next_frame += 2000;
        }
        return 0;
}
