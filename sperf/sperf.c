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

struct Syscall {
        char name[64];
        double time;
};
struct Syscall info[1024];
int h_info;
char tmp[1024], ttmp[1024], name[1024];

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
                h_info++;
        }
        return i;
}

#define clear() printf("\e[H\e[J\e[?25l")
#define show() printf("\e[?25h")

void sort(){
    for (int i=0;i<h_info;++i)
        for (int j=i+1;j<h_info;++j)
            if (info[i].time<info[j].time){
                struct Syscall tmp = info[i];
                info[i] = info[j];
                info[j] = tmp;
            }
}

#define RESET "\e[0m"
#define RED "\e[041m"
#define move(x,y) printf("\e[" #x ";" #y "m")

void draw_graph()
{
    int x = 10;
    int y = 10;
    move(x,y);
    for (int i=0; i<10; ++i){
        for (int j=0; j<10; ++j){
            printf(RED " " RESET);
        }
        printf("\n");
        }
}

void draw_table()
{
        printf("\e[H");
        double sum = 0;
        for (int i = 0; i < h_info; ++i) {
                printf("%s: %10lf\n", info[i].name, info[i].time);
                sum += info[i].time;
        }
        printf("******************\n");
        printf("total time: %10.5lf\n", sum);
        draw_graph();
}


void signal_callback_handler(int signum) {
    printf("    TERMINATED    \n");
    show();
    exit(signum);
}

int main(int argc, char *argv[], char *env[])
{
        //new argv
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
                                    draw_table();
                                    show();
                                    exit(0);
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
                                sort();
                                draw_table();
                                begin = now;
                        }
                }
        }

        return 0;
}
