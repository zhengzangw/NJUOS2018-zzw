#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/ioctl.h>

struct Syscall {
        char name[64];
        double time;
        int color;
};
struct Syscall info[1024];
int h_info;
char tmp[1024], ttmp[1024], name[1024];
int color_set[] = {41,42,43,44,45,46,47,101};
#define COLORNUM 7
int color_p = 0;
int ws_row, ws_col, ws_ratio;

int loc(char *name)
{
        int i;
        for (i = 0; i < h_info; ++i) {
                if (strcmp(name, info[i].name) == 0) {
                        break;
                }
        }
        if (i == h_info) {
                strcpy(info[i].name, name);
                info[i].color = color_set[color_p];
                color_p = (color_p + 1)%COLORNUM;
                h_info++;
        }
        return i;
}

#define clear() printf("\e[H\e[J\e[?25l")
#define show() printf("\e[?25h")

void sort()
{
        for (int i = 0; i < h_info; ++i)
                for (int j = i + 1; j < h_info; ++j)
                        if (info[i].time < info[j].time) {
                                struct Syscall tmp = info[i];
                                info[i] = info[j];
                                info[j] = tmp;
                        }
}

#define RESET "\e[0m"
#define move(x,y) printf("\e[%d;%dH", x, y)
#define draw_rect_label(x,y,s,t,num) draw_rect(x,y,s,t,num); draw_label(x,y,s,t,num)
double total_time,sum;

void draw_rect(int x, int y, int s, int t, int num){
    move(x, y);
    for (int i = 0; i <= s-x; ++i) {
    move(x + i, y);
        for (int j = 0; j <= t-y; ++j) {
            printf("\e[%dm " RESET, info[num].color);
        }
    printf("\n");
    }
}

void draw_label(int x, int y, int s, int t, int num){
    int len = strlen(info[num].name);
    move((s+x)/2, (t+y)/2-len/2);
    printf("\e[%dm%s" RESET, info[num].color, info[num].name);
    move((s+x)/2+1, (t+y)/2-4);
    printf("\e[%dm(%3.2lf%%)" RESET, info[num].color, info[num].time*100/sum);
    fflush(stdout);
}

void set_others(){
    strcpy(info[h_info].name, "others");
    info[h_info].time = sum - total_time;
    info[h_info].color = 101;
}

#define SX 1
#define SY (SX*ws_ratio)
#define X (ws_row*4/5)
#define Y (ws_col*4/5)
void clear_graph(){
    move(SX,SY);
    for (int i=0; i<=X+10; ++i){
        move(i, 0);
        for (int j=0; j<=Y+10; ++j)
            printf(RESET " ");
    }
}

void draw_graph()
{
    int x = SX;
    int y = SY;
    move(x,y);
    int odd = 0;
    clear_graph();
    sum = 0;
    for (int i = 0; i < h_info; ++i) {
      sum += info[i].time;
    }
    total_time = 0;
    for (int i=0;i<h_info;++i,odd^=1){
        if (total_time/sum>0.95) break;
        total_time += info[i].time;
        if (!odd){
          int w = (double)X*Y*info[i].time/sum/(X-x+1);
          draw_rect_label(x,y,X,y+w-3,i);
          y += w;
        } else {
          int w = (double)X*Y*info[i].time/sum/(Y-y+1);
          draw_rect_label(x,y,x+w-2,Y,i);
          x += w;
        }
    }
    set_others();
    draw_rect_label(x,y,X,Y,h_info);
}

void draw_table()
{
        printf("\e[H");
        sum = 0;
        for (int i = 0; i < h_info; ++i) {
                printf("%s: %10lf\n", info[i].name, info[i].time);
                sum += info[i].time;
        }
        printf("******************\n");
        printf("total time: %10.5lf\n", sum);
}

void signal_callback_handler(int signum)
{
        move(X+1,Y/2);
        printf("TERMINATED\n");
        show();
        exit(signum);
}

#ifdef TABLE
#define draw sort(); draw_table
#else
#define draw sort(); draw_graph
#endif


int main(int argc, char *argv[], char *env[])
{
        //new argv
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        ws_row = w.ws_row;
        ws_col = w.ws_col;
        ws_ratio = ws_col/ws_row;

        char *argv_new[argc + 2];
        argv_new[0] = "/usr/bin/strace";
        argv_new[1] = "-Txx";
        for (int i = 1; i < argc; ++i)
                argv_new[i + 1] = argv[i];
        argv_new[argc + 1] = NULL;
        time_t begin = time(NULL);
        signal(SIGINT, signal_callback_handler);

        int flides[2];
        if (pipe(flides) != 0) {
                perror("pipe failed");
                exit(EXIT_FAILURE);
        }
        int pid = fork();
        if (pid == 0) {
                dup2(flides[1], STDERR_FILENO);
                close(STDOUT_FILENO);
                execve("/usr/bin/strace", argv_new, env);
        } else {
                FILE *input = fdopen(flides[0], "r");
                clear();
                while (true) {
                        // read
                        memset(tmp, 0, sizeof(tmp));
                        do {
                                fgets(ttmp, 1024, input);
                                strcat(tmp, ttmp);

                                if (strncmp(ttmp, "/usr/bin/strace", 15) == 0) {
                                        printf("%s+++  Fail to run sperf +++\n",
                                               tmp);
                                        signal_callback_handler(1);
                                }
                                if (strncmp(ttmp, "+++", 3) == 0) {
                                        draw();
                                        signal_callback_handler(0);
                                }
                        } while (tmp[strlen(tmp) - 2] != '>');

                        // parse name
                        int t;
                        for (t = 0; t < strlen(tmp); ++t) {
                                if (tmp[t] == '(')
                                        break;
                        }
                        if (t == strlen(tmp))
                                continue;
                        strncpy(name, tmp, t);
                        name[t] = '\0';

                        // parse time
                        double dur;
                        for (t = strlen(tmp) - 1; t >= 0; --t) {
                                if (tmp[t] == '<')
                                        break;
                        }
                        if (t < 0)
                                continue;
                        sscanf(tmp + t + 1, "%lf", &dur);
                        info[loc(name)].time += dur;

                        // draw output
                        time_t now = time(NULL);
                        if (now - begin >= 1) {
                                draw();
                                begin = now;
                        }
                }
        }

        return 0;
}
