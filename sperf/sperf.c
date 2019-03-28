#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Syscall{
  char name[64];
  double time;
} info[1024];
int h_info;
char tmp[1024], name[1024];

int loc(char *name){
    int i;
    for (i=0;i<h_info;++i){
        if (strcmp(name, info[i].name)==0){
            break;
        }
    }
    if (i==h_info) {
        strcpy(info[i].name, name);
        h_info ++;
    }
    return i;
}

#define clear() printf("\e[?25l\e[2J\e[H")
#define show() printf("\e[?25h")

void draw_table(){
  clear();
  double sum = 0;
  for (int i=0;i<h_info;++i){
      fprintf(stdout, "%s: %10lf\n", info[i].name, info[i].time);
      sum += info[i].time;
  }
  fprintf(stdout, "SUM: %10lf\n", sum);
}


int main(int argc, char *argv[], char *env[]) {
  //new argv
  char *argv_new[argc+2];
  argv_new[0] = "/usr/bin/strace";
  argv_new[1] = "-Txx";
  for (int i=1;i<argc;++i) argv_new[i+1] = argv[i];
  argv_new[argc+1] = NULL;
  clock_t begin = clock();

  int flides[2];
  if (pipe(flides)!=0){
      perror("pipe failed");
      exit(EXIT_FAILURE);
  }
  int pid = fork();
  if (pid == 0){
     dup2(flides[1], STDERR_FILENO);
     close(STDOUT_FILENO);
     execve("/usr/bin/strace", argv_new, env);
  } else {
      FILE* input = fdopen(flides[0], "r");
      clear();

      while (true){
        fgets(tmp, 1024, input);

        if (strncmp(tmp, "/usr/bin/strace", 15)==0){
            printf("%s+++  Fail to run sperf +++\n", tmp);
            show();
            exit(1);
        }
        if (strncmp(tmp, "+++", 3)==0){
            break;
        }

        int t;
        for (t=0;t<strlen(tmp);++t){
            if (tmp[t]=='(') break;
        }
        strncpy(name, tmp, t);
        name[t] = '\0';

        double dur;
        for (t=strlen(tmp)-1;t>=0;--t){
            if (tmp[t]=='<') break;
        }
        sscanf(tmp+t+1,"%lf", &dur);
        info[loc(name)].time += dur;

        clock_t now = clock();
        if ((now - begin)/CLOCKS_PER_SEC>=1){
            draw_table();
            begin = now;
        }
      }
     draw_table();
     show();
  }

  return 0;
}
